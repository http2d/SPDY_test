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
#include "avl.h"


static ret_t
node_new (http2d_avl_node_t **node, void *key)
{
	HTTP2D_NEW_STRUCT (n, avl_node);

	http2d_avl_generic_node_init (AVL_GENERIC_NODE(n));

	http2d_buffer_init (&n->key);
	http2d_buffer_add_buffer (&n->key, BUF(key));

	*node = n;
	return ret_ok;
}

static ret_t
node_mrproper (http2d_avl_node_t *key)
{
	http2d_buffer_mrproper (&key->key);
	return ret_ok;
}

static ret_t
node_free (http2d_avl_node_t *key)
{
	ret_t ret;

	if (key == NULL)
		return ret_ok;

	ret = node_mrproper (key);
	free (key);
	return ret;
}

static int
node_cmp (http2d_avl_node_t *A,
	  http2d_avl_node_t *B,
	  http2d_avl_t      *avl)
{
        if (AVL(avl)->case_insensitive) {
                return http2d_buffer_case_cmp_buf (&A->key, &B->key);
        } else {
                return http2d_buffer_cmp_buf (&A->key, &B->key);
        }
}

static int
node_is_empty (http2d_avl_node_t *key)
{
	return http2d_buffer_is_empty (&key->key);
}


ret_t
http2d_avl_init (http2d_avl_t *avl)
{
	http2d_avl_generic_t *gen = AVL_GENERIC(avl);

	http2d_avl_generic_init (gen);

	gen->node_mrproper = (avl_gen_node_mrproper_t) node_mrproper;
	gen->node_cmp      = (avl_gen_node_cmp_t) node_cmp;
	gen->node_is_empty = (avl_gen_node_is_empty_t) node_is_empty;

	avl->case_insensitive = false;

	return ret_ok;
}

ret_t
http2d_avl_new (http2d_avl_t **avl)
{
	HTTP2D_NEW_STRUCT (n, avl);
	*avl = n;
	return http2d_avl_init (n);
}


ret_t
http2d_avl_add (http2d_avl_t *avl, http2d_buffer_t *key, void *value)
{
	ret_t                ret;
	http2d_avl_node_t *n    = NULL;

	ret = node_new (&n, key);
	if ((ret != ret_ok) || (n == NULL)) {
		node_free (n);
		return ret;
	}

	return http2d_avl_generic_add (AVL_GENERIC(avl), AVL_GENERIC_NODE(n), value);
}

ret_t
http2d_avl_del (http2d_avl_t *avl, http2d_buffer_t *key, void **value)
{
	http2d_avl_node_t tmp;

	http2d_buffer_fake (&tmp.key, key->buf, key->len);

	return http2d_avl_generic_del (AVL_GENERIC(avl), AVL_GENERIC_NODE(&tmp), value);
}

ret_t
http2d_avl_get (http2d_avl_t *avl, http2d_buffer_t *key, void **value)
{
	http2d_avl_node_t tmp;

	http2d_buffer_fake (&tmp.key, key->buf, key->len);

	return http2d_avl_generic_get (AVL_GENERIC(avl), AVL_GENERIC_NODE(&tmp), value);
}



ret_t
http2d_avl_add_ptr (http2d_avl_t *avl, const char *key, void *value)
{
	http2d_buffer_t tmp_key;

	http2d_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return http2d_avl_add (avl, &tmp_key, value);
}

ret_t
http2d_avl_del_ptr (http2d_avl_t *avl, const char *key, void **value)
{
	http2d_buffer_t tmp_key;

	http2d_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return http2d_avl_del (avl, &tmp_key, value);
}

ret_t
http2d_avl_get_ptr (http2d_avl_t *avl, const char *key, void **value)
{
	http2d_buffer_t tmp_key;

	http2d_buffer_fake (&tmp_key, (const char *)key, strlen(key));
	return http2d_avl_get (avl, &tmp_key, value);
}



static ret_t
while_func_wrap (http2d_avl_generic_node_t *node,
		 void                      *value,
		 void                      *params_internal)
{
	http2d_buffer_t          *key;
	void                    **params    = (void **)params_internal;
	http2d_avl_while_func_t   func_orig = params[0];
	void                     *param     = params[1];

	key = &AVL_NODE(node)->key;
	return func_orig (key, value, param);
}


ret_t
http2d_avl_while (http2d_avl_generic_t     *avl,
		  http2d_avl_while_func_t   func,
		  void                     *param,
		  http2d_buffer_t         **key,
		  void                    **value)
{
	void *params_internal[] = {func, param};

	return http2d_avl_generic_while (avl, while_func_wrap, params_internal,
					 (http2d_avl_generic_node_t **)key, (void**)value);
}


ret_t
http2d_avl_set_case (http2d_avl_t *avl,
		     bool          case_insensitive)
{
	avl->case_insensitive = case_insensitive;
	return ret_ok;
}
