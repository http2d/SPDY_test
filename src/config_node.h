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

#ifndef HTTP2D_CONFIG_NODE_H
#define HTTP2D_CONFIG_NODE_H

#include "common.h"
#include "list.h"
#include "buffer.h"
#include <stdbool.h>

typedef struct {
	   http2d_list_t    entry;
	   http2d_list_t    child;

	   http2d_buffer_t  key;
	   http2d_buffer_t  val;
} http2d_config_node_t;

#define CONFIG_NODE(c) ((http2d_config_node_t *)(c))

#define http2d_config_node_foreach(i,config) \
	list_for_each (i, &config->child)

#define CONFIG_KEY_IS(c,str) (http2d_buffer_case_cmp_str (&(c)->key, str) == 0)



typedef ret_t (* http2d_config_node_while_func_t) (http2d_config_node_t *, void *);
typedef ret_t (* http2d_config_node_list_func_t)  (char *, void *);

ret_t http2d_config_node_new       (http2d_config_node_t **conf);
ret_t http2d_config_node_free      (http2d_config_node_t  *conf);
ret_t http2d_config_node_init      (http2d_config_node_t  *conf);
ret_t http2d_config_node_mrproper  (http2d_config_node_t  *conf);

ret_t http2d_config_node_get       (http2d_config_node_t *conf, const char *key, http2d_config_node_t **entry);
ret_t http2d_config_node_get_buf   (http2d_config_node_t *conf, http2d_buffer_t *key, http2d_config_node_t **entry);

ret_t http2d_config_node_add_buf   (http2d_config_node_t *conf, http2d_buffer_t *key, http2d_buffer_t *val);
ret_t http2d_config_node_while     (http2d_config_node_t *conf, http2d_config_node_while_func_t func, void *data);

/* Convenience functions: value retrieving
 */
ret_t http2d_config_node_read       (http2d_config_node_t *conf, const char *key, http2d_buffer_t **buf);
ret_t http2d_config_node_copy       (http2d_config_node_t *conf, const char *key, http2d_buffer_t  *buf);

ret_t http2d_config_node_read_path  (http2d_config_node_t *conf, const char *key, http2d_buffer_t **buf);
ret_t http2d_config_node_read_int   (http2d_config_node_t *conf, const char *key, int *num);
ret_t http2d_config_node_read_long  (http2d_config_node_t *conf, const char *key, long *num);
ret_t http2d_config_node_read_bool  (http2d_config_node_t *conf, const char *key, bool *val);
ret_t http2d_config_node_read_list  (http2d_config_node_t *conf, const char *key,
				     http2d_config_node_list_func_t func, void *param);

ret_t http2d_config_node_convert_list (http2d_config_node_t *conf, const char *key, http2d_list_t *list);

#endif /* HTTP2D_CONFIG_NODE_H */
