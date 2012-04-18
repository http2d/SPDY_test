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

#ifndef HTTP2D_AVL_H
#define HTTP2D_AVL_H

#include "buffer.h"
#include "avl_generic.h"

/* AVL Tree Node
 */
typedef struct {
	http2d_avl_generic_node_t base;
	http2d_buffer_t           key;
} http2d_avl_node_t;

/* AVL Tree
 */
typedef struct {
	http2d_avl_generic_t base;
	bool                 case_insensitive;
} http2d_avl_t;


#define AVL(a) ((http2d_avl_t *)(a))
#define AVL_NODE(a) ((http2d_avl_node_t *)(a))


ret_t http2d_avl_init     (http2d_avl_t  *avl);
ret_t http2d_avl_new      (http2d_avl_t **avl);
ret_t http2d_avl_set_case (http2d_avl_t  *avl, bool case_insensitive);

ret_t http2d_avl_add      (http2d_avl_t *avl, http2d_buffer_t *key, void  *value);
ret_t http2d_avl_del      (http2d_avl_t *avl, http2d_buffer_t *key, void **value);
ret_t http2d_avl_get      (http2d_avl_t *avl, http2d_buffer_t *key, void **value);

ret_t http2d_avl_add_ptr  (http2d_avl_t *avl, const char *key, void  *value);
ret_t http2d_avl_del_ptr  (http2d_avl_t *avl, const char *key, void **value);
ret_t http2d_avl_get_ptr  (http2d_avl_t *avl, const char *key, void **value);


typedef ret_t (* http2d_avl_while_func_t) (http2d_buffer_t *key, void *value, void *param);

ret_t http2d_avl_while (http2d_avl_generic_t       *avl,
			http2d_avl_while_func_t     func,
			void                       *param,
			http2d_buffer_t           **key,
			void                      **value);


#endif /* HTTP2D_AVL_H */
