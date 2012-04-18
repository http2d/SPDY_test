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

#ifndef HTTP2D_RULE_H
#define HTTP2D_RULE_H

#include "common.h"
#include "buffer.h"
#include "list.h"
#include "config_entry.h"

#define HTTP2D_RULE_PRIO_NONE    0
#define HTTP2D_RULE_PRIO_DEFAULT 1

/* Callback function definitions
 */
typedef ret_t (* rule_func_new_t)       (void **rule);
typedef ret_t (* rule_func_configure_t) (void  *rule, http2d_config_node_t *conf, void *vsrv);
typedef ret_t (* rule_func_match_t)     (void  *rule, void *cnt, void *ret_conf);

/* Data types
 */
struct http2d_rule {
	http2d_module_t       module;

	/* Properties */
	http2d_list_t         list_node;
	http2d_config_entry_t config;

	http2d_boolean_t      final;
	cuint_t                 priority;
	struct http2d_rule   *parent_rule;

	/* Virtual methods */
	rule_func_match_t       match;
	rule_func_configure_t   configure;
};

typedef struct http2d_rule http2d_rule_t;
#define RULE(x) ((http2d_rule_t *)(x))

/* Easy initialization
 */
#define PLUGIN_INFO_RULE_EASY_INIT(name)                         \
	PLUGIN_INFO_INIT(name, http2d_rule,                    \
		(void *)http2d_rule_ ## name ## _new,          \
		(void *)NULL)

#define PLUGIN_INFO_RULE_EASIEST_INIT(name)                      \
	PLUGIN_EMPTY_INIT_FUNCTION(name)                         \
	PLUGIN_INFO_RULE_EASY_INIT(name)

/* Methods
 */
ret_t http2d_rule_free        (http2d_rule_t *rule);

/* Rule methods
 */
ret_t http2d_rule_init_base   (http2d_rule_t *rule, http2d_plugin_info_t *info);

/* Rule virtual methods
 */
ret_t http2d_rule_match       (http2d_rule_t *rule, void *cnt, void *ret_conf);
ret_t http2d_rule_configure   (http2d_rule_t *rule, http2d_config_node_t *conf, void *vsrv);

/* Utilities
 */
void http2d_rule_get_config  (http2d_rule_t *rule, http2d_config_entry_t **config);

#endif /* HTTP2D_RULE_H */
