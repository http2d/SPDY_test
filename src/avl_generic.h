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

#ifndef HTTP2D_AVL_GENERIC_H
#define HTTP2D_AVL_GENERIC_H

#include "buffer.h"

/* AVL Tree Node base
 */

typedef struct http2d_avl_generic_node http2d_avl_generic_node_t;

struct http2d_avl_generic_node {
	short                           balance;
	struct http2d_avl_generic_node *left;
	struct http2d_avl_generic_node *right;
	bool                            left_child;
	bool                            right_child;
	void                           *value;
};

#define AVL_GENERIC_NODE(a) ((http2d_avl_generic_node_t *)(a))

ret_t http2d_avl_generic_node_init (http2d_avl_generic_node_t *node);


/* AVL Tree
 */

typedef struct http2d_avl_generic http2d_avl_generic_t;

typedef ret_t (*avl_gen_node_mrproper_t) (http2d_avl_generic_node_t *);
typedef ret_t (*avl_gen_node_cmp_t)      (http2d_avl_generic_node_t *, http2d_avl_generic_node_t *, http2d_avl_generic_t *);
typedef ret_t (*avl_gen_node_is_empty_t) (http2d_avl_generic_node_t *);


struct http2d_avl_generic {
	http2d_avl_generic_node_t *root;

	/* Virtual methods */
	avl_gen_node_mrproper_t node_mrproper;
	avl_gen_node_cmp_t      node_cmp;
	avl_gen_node_is_empty_t node_is_empty;
};

#define AVL_GENERIC(a) ((http2d_avl_generic_t *)(a))

ret_t http2d_avl_generic_init (http2d_avl_generic_t *avl);


/* Public Methods
 */
typedef ret_t (* http2d_avl_generic_while_func_t) (http2d_avl_generic_node_t *key, void *value, void *param);

ret_t http2d_avl_mrproper (http2d_avl_generic_t *avl, http2d_func_free_t free_func);
ret_t http2d_avl_free     (http2d_avl_generic_t *avl, http2d_func_free_t free_func);
ret_t http2d_avl_check    (http2d_avl_generic_t *avl);
ret_t http2d_avl_len      (http2d_avl_generic_t *avl, size_t *len);
int   http2d_avl_is_empty (http2d_avl_generic_t *avl);

ret_t http2d_avl_generic_add (http2d_avl_generic_t *avl, http2d_avl_generic_node_t *key, void  *value);
ret_t http2d_avl_generic_del (http2d_avl_generic_t *avl, http2d_avl_generic_node_t *key, void **value);
ret_t http2d_avl_generic_get (http2d_avl_generic_t *avl, http2d_avl_generic_node_t *key, void **value);

ret_t http2d_avl_generic_while (http2d_avl_generic_t             *avl,
				http2d_avl_generic_while_func_t   func,
				void                               *param,
				http2d_avl_generic_node_t       **key,
				void                              **value);

#endif /* HTTP2D_AVL_GENERIC_H */
