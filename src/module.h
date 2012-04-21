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

#ifndef CHEROKEE_MODULE_H
#define CHEROKEE_MODULE_H

#include "common.h"
#include "plugin.h"
#include "config_node.h"
#include "server.h"

/* Callback function prototipes
 */
typedef void   * module_func_init_t;
typedef ret_t (* module_func_new_t)        (void *);
typedef ret_t (* module_func_free_t)       (void *);
typedef ret_t (* module_func_props_free_t) (void *);
typedef ret_t (* module_func_configure_t)  (http2d_config_node_t *conf, http2d_server_t *srv, void **props);


/* Module properties
 */
typedef struct {
	module_func_props_free_t  free;
} http2d_module_props_t;

#define MODULE_PROPS(x)      ((http2d_module_props_t *)(x))
#define MODULE_PROPS_FREE(f) ((module_func_props_free_t)(f))


/* Data types for module objects
 */
typedef struct {
	http2d_plugin_info_t   *info;       /* ptr to info structure    */
	http2d_module_props_t  *props;      /* ptr to local properties  */

	module_func_new_t         instance;   /* constructor              */
	void                     *init;       /* initializer              */
	module_func_free_t        free;       /* destructor               */
} http2d_module_t;

#define MODULE(x) ((http2d_module_t *) (x))


/* Methods
 */
ret_t http2d_module_init_base (http2d_module_t *module, http2d_module_props_t *props, http2d_plugin_info_t *info);
ret_t http2d_module_get_name  (http2d_module_t *module, const char **name);

/* Property methods
 */
ret_t http2d_module_props_init_base (http2d_module_props_t *prop, module_func_props_free_t free_func);
ret_t http2d_module_props_free_base (http2d_module_props_t *prop);
ret_t http2d_module_props_free      (http2d_module_props_t *prop);

#endif /* HTTP2D_MODULE_H */
