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

#ifndef HTTP2D_CACHE_H
#define HTTP2D_CACHE_H

#include "common.h"
#include "list.h"
#include "avl.h"
#include "config_node.h"

/* Forward declaration */
typedef struct http2d_cache       http2d_cache_t;
typedef struct http2d_cache_priv  http2d_cache_priv_t;
typedef struct http2d_cache_entry http2d_cache_entry_t;

/* Callback prototypes */
typedef ret_t (* http2d_cache_new_func_t)  (struct http2d_cache        *cache,
					      http2d_buffer_t            *key,
					      void                         *param,
					      struct http2d_cache_entry **ret);

typedef ret_t (* http2d_cache_get_stats_t) (struct http2d_cache        *cache,
					      http2d_buffer_t            *key);

typedef ret_t (* http2d_cache_entry_clean_t) (struct http2d_cache_entry *entry);
typedef ret_t (* http2d_cache_entry_fetch_t) (struct http2d_cache_entry *entry);
typedef ret_t (* http2d_cache_entry_free_t)  (struct http2d_cache_entry *entry);

/* Enums */
typedef enum {
	cache_no_list = 0,
	cache_t1,
	cache_t2,
	cache_b1,
	cache_b2
} http2d_cache_list_t;

/* Classes */
struct http2d_cache {
	/* Lookup table */
	http2d_avl_t  map;

	/* LRU (Least Recently Used)   */
	http2d_list_t _t1;
	http2d_list_t _b1;
	cint_t          len_t1;
	cint_t          len_b1;

	/* LFU (Least Frequently Used) */
	http2d_list_t _t2;
	http2d_list_t _b2;
	cint_t          len_t2;
	cint_t          len_b2;

	/* Configuration */
	cint_t          max_size;
	cint_t          target_t1;

	/* Stats */
	cuint_t         count;
	cuint_t         count_hit;
	cuint_t         count_miss;

	/* Callbacks */
	http2d_cache_new_func_t  new_cb;
	void                      *new_cb_param;
	http2d_cache_get_stats_t stats_cb;

	/* Private properties */
	http2d_cache_priv_t     *priv;
};

struct http2d_cache_entry {
	/* Internal stuff */
	http2d_list_t              listed;
	http2d_buffer_t            key;
	http2d_cache_list_t        in_list;

	cint_t                       ref_count;
	void                        *mutex;
	http2d_cache_t            *cache;

	/* Callbacks */
	http2d_cache_entry_clean_t clean_cb;
	http2d_cache_entry_fetch_t fetch_cb;
	http2d_cache_entry_free_t  free_cb;
};

/* Castings */
#define CACHE(x)       ((http2d_cache_t *)(x))
#define CACHE_ENTRY(x) ((http2d_cache_entry_t *)(x))


/* Cache Entries
 */
ret_t http2d_cache_entry_init  (http2d_cache_entry_t  *entry,
				  http2d_buffer_t       *key,
				  http2d_cache_t        *cache,
				  void                    *mutex);
ret_t http2d_cache_entry_unref (http2d_cache_entry_t **entry);

/* Cache Objects
 */
ret_t http2d_cache_init      (http2d_cache_t *cache);
ret_t http2d_cache_mrproper  (http2d_cache_t *cache);
ret_t http2d_cache_clean     (http2d_cache_t *cache);

/* Cache Functionality
 */
ret_t http2d_cache_configure (http2d_cache_t        *cache,
				http2d_config_node_t  *conf);

ret_t http2d_cache_get       (http2d_cache_t        *cache,
				http2d_buffer_t       *key,
				http2d_cache_entry_t **entry);

ret_t http2d_cache_get_stats (http2d_cache_t        *cache,
				http2d_buffer_t       *info);

#endif /* HTTP2D_CACHE_H */
