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

#ifndef CHEROKEE_PLUGIN_H
#define CHEROKEE_PLUGIN_H

#include "http.h"

/* Plug-in types
 */
typedef enum {
	http2d_generic   = 1,
	http2d_logger    = 1 << 1,
	http2d_handler   = 1 << 2,
	http2d_encoder   = 1 << 3,
	http2d_validator = 1 << 4,
	http2d_balancer  = 1 << 5,
	http2d_rule      = 1 << 6,
	http2d_vrule     = 1 << 7,
	http2d_cryptor   = 1 << 8,
	http2d_collector = 1 << 9
} http2d_plugin_type_t;


/* Generic Plug-in info structure
 */
typedef struct {
	http2d_plugin_type_t    type;
	void                     *instance;
	void                     *configure;
	const char               *name;
} http2d_plugin_info_t;

#define PLUGIN_INFO(x) ((http2d_plugin_info_t *)(x))


/* Specific Plug-ins structures
 */
typedef struct {
	http2d_plugin_info_t    plugin;
	http2d_http_method_t    valid_methods;
} http2d_plugin_info_handler_t;

typedef struct {
	http2d_plugin_info_t    plugin;
	http2d_http_auth_t      valid_methods;
} http2d_plugin_info_validator_t;

#define PLUGIN_INFO_HANDLER(x)     ((http2d_plugin_info_handler_t *)(x))
#define PLUGIN_INFO_VALIDATOR(x)   ((http2d_plugin_info_validator_t *)(x))

/* Commodity macros
 */
#define PLUGIN_INFO_NAME(name)       http2d_ ## name ## _info
#define PLUGIN_INFO_PTR(name)        PLUGIN_INFO(&PLUGIN_INFO_NAME(name))
#define PLUGIN_INFO_HANDLER_PTR(x)   PLUGIN_INFO_HANDLER(&PLUGIN_INFO_NAME(x))
#define PLUGIN_INFO_VALIDATOR_PTR(x) PLUGIN_INFO_VALIDATOR(&PLUGIN_INFO_NAME(x))


/* Convenience macros
 */
#define PLUGIN_INFO_INIT(name, type, func, conf)                    \
 	http2d_plugin_info_t                                      \
	        PLUGIN_INFO_NAME(name) =   	                    \
		{	type,	 		  	  	    \
			func,		 			    \
			conf,                                       \
			#name                                       \
		}

#define PLUGIN_INFO_HANDLER_INIT(name, type, func, conf, methods)   \
 	http2d_plugin_info_handler_t                              \
	        PLUGIN_INFO_NAME(name) = {  	                    \
		{	type,	 		  	  	    \
			func,		 			    \
			conf,                                       \
			#name                                       \
		},			                            \
		(methods)				  	    \
 	}

#define PLUGIN_INFO_VALIDATOR_INIT(name, type, func, conf, methods) \
 	http2d_plugin_info_validator_t                            \
	        PLUGIN_INFO_NAME(name) = {	                    \
		{	type,	 				    \
			func,		 			    \
			conf,                                       \
			#name                                       \
		},				 		    \
		(methods)				  	    \
 	}


/* Easy init macros
 */
#define PLUGIN_INFO_EASY_INIT(type,name)                            \
	PLUGIN_INFO_INIT(name, type,                                \
		(void *)type ## _ ## name ## _new,     \
		(void *)type ## _ ## name ## _configure)


/* Plugin initialization function
 */
#define PLUGIN_INIT_NAME(name)  http2d_plugin_ ## name ## _init
#define PLUGIN_IS_INIT(name)    _## name ##_is_init

#define PLUGIN_INIT_PROTOTYPE(name)         			    \
	static http2d_boolean_t PLUGIN_IS_INIT(name) = false;     \
        void                                                        \
	PLUGIN_INIT_NAME(name) (http2d_plugin_loader_t *loader)

#define PLUGIN_INIT_ONCE_CHECK(name)         		            \
	if (PLUGIN_IS_INIT(name))				    \
		return;						    \
	PLUGIN_IS_INIT(name) = true

#define PLUGIN_EMPTY_INIT_FUNCTION(name)                            \
	void                                                        \
	PLUGIN_INIT_NAME(name) (http2d_plugin_loader_t *loader)   \
	{                                                           \
		UNUSED(loader);					    \
	}                                                           \


#endif /* HTTP2D_PLUGIN_H */
