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
#include "module.h"

ret_t
http2d_module_init_base (http2d_module_t       *module,
			   http2d_module_props_t *props,
			   http2d_plugin_info_t  *info)
{
	/* Properties
	 */
	module->info     = info;
	module->props    = props;

	/* Virtual methods
	 */
	module->instance = NULL;
	module->init     = NULL;
	module->free     = NULL;

	return ret_ok;
}


ret_t
http2d_module_get_name (http2d_module_t *module, const char **name)
{
	if (module->info == NULL)
		return ret_not_found;

	if (module->info->name == NULL) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	*name = module->info->name;
	return ret_ok;
}



/* Module properties
 */
ret_t
http2d_module_props_init_base (http2d_module_props_t *prop, module_func_props_free_t free_func)
{
	prop->free = free_func;
	return ret_ok;
}


ret_t
http2d_module_props_free (http2d_module_props_t *prop)
{
	if (prop == NULL)
		return ret_error;

	if (prop->free == NULL) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	prop->free (prop);
	return ret_ok;
}


ret_t
http2d_module_props_free_base (http2d_module_props_t *prop)
{
	free (prop);
	return ret_ok;
}
