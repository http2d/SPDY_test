/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* All files in http2d are Copyright (C) 2012 Alvaro Lopez Ortega.
 *
 *   Authors:
 *     * Alvaro Lopez Ortega <alvaro@alobbs.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "common-internal.h"
#include "plugin_loader.h"

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_DLFCN_H
#  include <dlfcn.h>
#endif

#include "buffer.h"

typedef http2d_plugin_loader_entry_t entry_t;

static pthread_mutex_t dlerror_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef HAVE_RTLDNOW
# define RTLD_BASE RTLD_NOW
#else
# define RTLD_BASE RTLD_LAZY
#endif

static ret_t
add_static_entry (http2d_plugin_loader_t *loader,
		  const char             *name,
		  void                   *info)
{
	entry_t *entry;

	entry = malloc (sizeof(entry_t));
	if (entry == NULL) {
		return ret_nomem;
	}

	entry->dlopen_ref = dlopen (NULL, RTLD_BASE);
	entry->info       = info;

	http2d_avl_add_ptr (&loader->table, (char *)name, entry);
	return ret_ok;
}


static void
free_entry (void *item)
{
	entry_t *entry = (entry_t *)item;

	if (entry->dlopen_ref) {
		dlclose (entry->dlopen_ref);
		entry->dlopen_ref = NULL;
	}

	free (item);
}


ret_t
http2d_plugin_loader_init (http2d_plugin_loader_t *loader)
{
	ret_t ret;

	ret = http2d_avl_init (&loader->table);
	if (unlikely(ret < ret_ok))
		return ret;

	/* Plug-in dir
	 */
	ret = http2d_buffer_init (&loader->module_dir);
	if (unlikely(ret < ret_ok))
		return ret;

	http2d_buffer_add_str (&loader->module_dir, HTTP2D_PLUGINDIR);

	/* Plug-in dependencies dir
	 */
	ret = http2d_buffer_init (&loader->deps_dir);
	if (unlikely(ret < ret_ok))
		return ret;

	http2d_buffer_add_str (&loader->deps_dir, HTTP2D_DEPSDIR);

	return ret_ok;
}


ret_t
http2d_plugin_loader_mrproper (http2d_plugin_loader_t *loader)
{
	http2d_buffer_mrproper (&loader->module_dir);
	http2d_buffer_mrproper (&loader->deps_dir);

	http2d_avl_mrproper (AVL_GENERIC(&loader->table), free_entry);
	return ret_ok;
}


static void *
get_sym_from_dlopen_handler (void *dl_handle, const char *sym)
{
	void       *re;
	const char *error;

	/* Clear the possible error and look for the symbol
	 */

	HTTP2D_MUTEX_LOCK (&dlerror_mutex);
	dlerror();
	re = (void *) dlsym(dl_handle, sym);
	if ((error = dlerror()) != NULL)  {
//		LOG_ERROR (HTTP2D_ERROR_PLUGIN_LOAD_NO_SYM, sym, error);
		HTTP2D_MUTEX_UNLOCK (&dlerror_mutex);
		return NULL;
	}
	HTTP2D_MUTEX_UNLOCK (&dlerror_mutex);

	return re;
}


static ret_t
dylib_open (http2d_plugin_loader_t  *loader,
	    const char                *libname,
	    int                        extra_flags,
	    void                     **handler_out)
{
	ret_t             ret;
	void             *lib;
	int               flags;
	http2d_buffer_t tmp = HTTP2D_BUF_INIT;

	flags = RTLD_BASE | extra_flags;

	/* Build the path string
	 */
	ret = http2d_buffer_add_va (&tmp, "%s/libplugin_%s." MOD_SUFFIX, loader->module_dir.buf, libname);
	if (unlikely(ret < ret_ok)) {
		http2d_buffer_mrproper (&tmp);
		return ret;
	}
	/* Open the library
	 */
	HTTP2D_MUTEX_LOCK (&dlerror_mutex);
	lib = dlopen (tmp.buf, flags);
	if (lib == NULL) {
//		LOG_ERROR (HTTP2D_ERROR_PLUGIN_DLOPEN, dlerror(), tmp.buf);
		HTTP2D_MUTEX_UNLOCK (&dlerror_mutex);
		http2d_buffer_mrproper (&tmp);
		return ret_error;
	}
	HTTP2D_MUTEX_UNLOCK (&dlerror_mutex);

	/* Free the memory
	 */
	http2d_buffer_mrproper (&tmp);

	*handler_out = lib;
	return ret_ok;
}


static ret_t
execute_init_func (http2d_plugin_loader_t *loader,
		   const char               *module,
		   entry_t                  *entry)
{
	ret_t ret;
	void (*init_func) (http2d_plugin_loader_t *);
	http2d_buffer_t init_name = HTTP2D_BUF_INIT;

	/* Build the init function name
	 */
	ret = http2d_buffer_add_va (&init_name, "http2d_plugin_%s_init", module);
	if (unlikely(ret < ret_ok)) {
		http2d_buffer_mrproper (&init_name);
		return ret;
	}

	/* Get the function
	 */
	if (entry->dlopen_ref == NULL) {
		http2d_buffer_mrproper (&init_name);
		return ret_error;
	}

	init_func = get_sym_from_dlopen_handler (entry->dlopen_ref, init_name.buf);

	/* Only try to execute if it exists
	 */
	if (init_func == NULL) {
//		LOG_WARNING (HTTP2D_ERROR_PLUGIN_NO_INIT, init_name.buf);
		http2d_buffer_mrproper (&init_name);
		return ret_not_found;
	}

	/* Free the init function name string
	 */
	http2d_buffer_mrproper (&init_name);

	/* Execute the init func
	 */
	init_func(loader);
	return ret_ok;
}


static ret_t
get_info (http2d_plugin_loader_t  *loader,
	  const char                *module,
	  int                        flags,
	  http2d_plugin_info_t   **info,
	  void                     **dl_handler)
{
	ret_t             ret;
	http2d_buffer_t info_name = HTTP2D_BUF_INIT;

	/* Build the info struct string
	 */
	http2d_buffer_add_va (&info_name, "http2d_%s_info", module);

	/* Open it
	 */
	ret = dylib_open (loader, module, flags, dl_handler);
	if (ret != ret_ok) {
		http2d_buffer_mrproper (&info_name);
		return ret_error;
	}

	*info = get_sym_from_dlopen_handler (*dl_handler, info_name.buf);
	if (*info == NULL) {
		http2d_buffer_mrproper (&info_name);
		return ret_not_found;
	}

	/* Free the info struct string
	 */
	http2d_buffer_mrproper (&info_name);
	return ret_ok;
}


static ret_t
check_deps_file (http2d_plugin_loader_t *loader,
		 const char               *modname)
{
	FILE             *file;
	char              temp[128];
	http2d_buffer_t filename = HTTP2D_BUF_INIT;

	http2d_buffer_add_va (&filename, "%s/%s.deps", loader->deps_dir.buf, modname);
	file = fopen (filename.buf, "r");
	if (file == NULL)
		goto exit;

	while (!feof(file)) {
		int   len;
		char *ret;

		ret = fgets (temp, 127, file);
		if (ret == NULL)
			break;

		len = strlen (temp);

		if (len < 2)
			continue;
		if (temp[0] == '#')
			continue;

		if (temp[len-1] == '\n')
			temp[len-1] = '\0';

		http2d_plugin_loader_load (loader, temp);
		temp[0] = '\0';
	}

	fclose (file);

exit:
	http2d_buffer_mrproper (&filename);
	return ret_ok;
}


static ret_t
load_common (http2d_plugin_loader_t *loader,
	     const char               *modname,
	     int                       flags)
{
	ret_t                   ret;
	entry_t                *entry     = NULL;
	http2d_plugin_info_t *info      = NULL;
	void                   *dl_handle = NULL;

	/* If it is already loaded just return
	 */
	ret = http2d_avl_get_ptr (&loader->table, modname, (void **)&entry);
	if (ret == ret_ok)
		return ret_ok;

	/* Check deps
	 */
	ret = check_deps_file (loader, modname);
	if (ret != ret_ok)
		return ret;

	/* Get the module info
	 */
	ret = get_info (loader, modname, flags, &info, &dl_handle);
	switch (ret) {
	case ret_ok:
		break;
	case ret_error:
//		LOG_ERROR (HTTP2D_ERROR_PLUGIN_NO_OPEN, modname);
		return ret;
	case ret_not_found:
//		LOG_ERROR (HTTP2D_ERROR_PLUGIN_NO_INFO, modname);
		return ret;
	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	/* Add new entry
	 */
	entry = malloc (sizeof(entry_t));
	if (entry == NULL) {
		return ret_nomem;
	}

	entry->dlopen_ref = dl_handle;
	entry->info       = info;

	ret = http2d_avl_add_ptr (&loader->table, modname, entry);
	if (unlikely(ret != ret_ok)) {
		dlclose (entry->dlopen_ref);
		free(entry);
		return ret;
	}

	/* Execute init function
	 */
	ret = execute_init_func (loader, modname, entry);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}


ret_t
http2d_plugin_loader_load_no_global (http2d_plugin_loader_t *loader,
				       const char               *modname)
{
	return load_common (loader, modname, 0);
}


ret_t
http2d_plugin_loader_load (http2d_plugin_loader_t *loader,
			     const char               *modname)
{
#ifdef HAVE_RTLDGLOBAL
	return load_common (loader, modname, RTLD_GLOBAL);
#else
	return load_common (loader, modname, 0);
#endif
}


ret_t
http2d_plugin_loader_unload (http2d_plugin_loader_t *loader,
			       const char               *modname)
{
	int      re     = 0;
	ret_t    ret;
	entry_t *entry;

	/* Remove item from the table
	 */
	ret = http2d_avl_del_ptr (&loader->table, modname, (void **)&entry);
	if (ret != ret_ok)
		return ret;

	/* Free the resources
	 */
	if (entry->dlopen_ref != NULL) {
		re = dlclose (entry->dlopen_ref);
	}

	free (entry);

	return (re == 0) ? ret_ok : ret_error;
}


ret_t
http2d_plugin_loader_get_info (http2d_plugin_loader_t  *loader,
				 const char                *modname,
				 http2d_plugin_info_t   **info)
{
	ret_t    ret;
	entry_t *entry;

	ret = http2d_avl_get_ptr (&loader->table, modname, (void **)&entry);
	if (ret != ret_ok)
		return ret;

	if (entry == NULL)
		return ret_error;

	*info = entry->info;
	return ret_ok;
}


ret_t
http2d_plugin_loader_get_sym  (http2d_plugin_loader_t  *loader,
				 const char                *modname,
				 const char                *name,
				 void                     **sym)
{
	ret_t    ret;
	entry_t *entry;
	void    *tmp;

	/* Get the symbol from a dynamic library
	 */
	ret = http2d_avl_get_ptr (&loader->table, modname, (void **)&entry);
	if (ret != ret_ok)
		return ret;

	/* Even if we're trying to look for symbols in the executable,
	 * using dlopen(NULL), the handler pointer should not be nil.
	 */
	if ((entry == NULL) || (entry->dlopen_ref == NULL))
		return ret_not_found;

	tmp = get_sym_from_dlopen_handler (entry->dlopen_ref, name);
	if (tmp == NULL)
		return ret_not_found;

	*sym = tmp;
	return ret_ok;
}


ret_t
http2d_plugin_loader_get (http2d_plugin_loader_t  *loader,
			    const char                *modname,
			    http2d_plugin_info_t   **info)
{
	ret_t ret;

	ret = http2d_plugin_loader_load (loader, modname);
	if (ret != ret_ok)
		return ret;

	ret = http2d_plugin_loader_get_info (loader, modname, info);
	if (ret != ret_ok)
		return ret;

	return ret_ok;
}


ret_t
http2d_plugin_loader_set_directory  (http2d_plugin_loader_t *loader, http2d_buffer_t *dir)
{
	http2d_buffer_clean (&loader->module_dir);
	http2d_buffer_add_buffer (&loader->module_dir, dir);

	return ret_ok;
}


ret_t
http2d_plugin_loader_set_deps_dir (http2d_plugin_loader_t *loader, http2d_buffer_t *dir)
{
	http2d_buffer_clean (&loader->deps_dir);
	http2d_buffer_add_buffer (&loader->deps_dir, dir);

	return ret_ok;
}

