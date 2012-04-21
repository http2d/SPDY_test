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

#ifndef HTTP2D_PLUGIN_LOADER_H
#define HTTP2D_PLUGIN_LOADER_H

#include "common.h"
#include "module.h"
#include "avl.h"
#include "plugin.h"

typedef struct {
	http2d_plugin_info_t *info;
	void                 *dlopen_ref;
} http2d_plugin_loader_entry_t;

typedef struct {
	http2d_avl_t          table;
	http2d_buffer_t       module_dir;
	http2d_buffer_t       deps_dir;
} http2d_plugin_loader_t;

#define MODINFO(x)   ((http2d_module_info_t *) (x))
#define MODLOADER(x) ((http2d_plugin_loader_t *) (x))


ret_t http2d_plugin_loader_init           (http2d_plugin_loader_t *loader);
ret_t http2d_plugin_loader_mrproper       (http2d_plugin_loader_t *loader);

ret_t http2d_plugin_loader_set_directory  (http2d_plugin_loader_t *loader, http2d_buffer_t *dir);
ret_t http2d_plugin_loader_set_deps_dir   (http2d_plugin_loader_t *loader, http2d_buffer_t *dir);
ret_t http2d_plugin_loader_load           (http2d_plugin_loader_t *loader, const char *modname);
ret_t http2d_plugin_loader_load_no_global (http2d_plugin_loader_t *loader, const char *modname);
ret_t http2d_plugin_loader_unload         (http2d_plugin_loader_t *loader, const char *modname);

ret_t http2d_plugin_loader_get            (http2d_plugin_loader_t *loader, const char *modname, http2d_plugin_info_t **info);
ret_t http2d_plugin_loader_get_info       (http2d_plugin_loader_t *loader, const char *modname, http2d_plugin_info_t **info);
ret_t http2d_plugin_loader_get_sym        (http2d_plugin_loader_t *loader, const char *modname, const char *name, void **sym);

#endif /* HTTP2D_PLUGIN_LOADER_H */
