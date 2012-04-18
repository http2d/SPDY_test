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

#ifndef _WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

#include "resolv_cache.h"
#include "socket_lowlevel.h"
#include "util.h"
#include "avl.h"
#include "socket.h"
#include "error_log.h"

#define ENTRIES "resolve"


typedef struct {
	struct addrinfo    *addr;
	http2d_buffer_t     ip_str;
	http2d_buffer_t     ip_str_all;
} http2d_resolv_cache_entry_t;

struct http2d_resolv_cache {
	http2d_avl_t     table;
	HTTP2D_RWLOCK_T (lock);
};

static http2d_resolv_cache_t *__global_resolv = NULL;


/* Entries
 */
static ret_t
entry_new (http2d_resolv_cache_entry_t **entry)
{
	HTTP2D_NEW_STRUCT(n, resolv_cache_entry);

	n->addr = NULL;
	http2d_buffer_init (&n->ip_str);
	http2d_buffer_init (&n->ip_str_all);

	*entry = n;
	return ret_ok;
}


static void
entry_free (void *entry)
{
	http2d_resolv_cache_entry_t *e = entry;

	if (e->addr) {
		freeaddrinfo (e->addr);
	}

	http2d_buffer_mrproper (&e->ip_str);
	http2d_buffer_mrproper (&e->ip_str_all);
	free(entry);
}


static ret_t
entry_fill_up (http2d_resolv_cache_entry_t *entry,
	       http2d_buffer_t             *domain)
{
	ret_t            ret;
	char             tmp[46];       // Max IPv6 length is 45
	struct addrinfo *addr;
	time_t           eagain_at = 0;

	while (true) {
		ret = http2d_gethostbyname (domain, &entry->addr);
		if (ret == ret_ok) {
			break;

		} else if (ret == ret_eagain) {
			if (eagain_at == 0) {
				eagain_at = HTTP2D_BOGONOW;

			} else if (HTTP2D_BOGONOW > eagain_at + 3) {
			      	LOG_WARNING (HTTP2D_ERROR_RESOLVE_TIMEOUT, domain->buf);
				return ret_error;
			}

			HTTP2D_THREAD_YIELD;
			continue;

		} else {
			return ret_error;
		}
	}

	if (unlikely (entry->addr == NULL)) {
		return ret_error;
	}

	/* Render the text representation
	 */
	ret = http2d_ntop (entry->addr->ai_family, entry->addr->ai_addr, tmp, sizeof(tmp));
	if (ret != ret_ok) {
		return ret_error;
	}

	http2d_buffer_add (&entry->ip_str, tmp, strlen(tmp));

	/* Render the text representation (all the IPs)
	 */
	http2d_buffer_add_buffer (&entry->ip_str_all, &entry->ip_str);

	addr = entry->addr;
	while (addr != NULL) {
		ret = http2d_ntop (entry->addr->ai_family, addr->ai_addr, tmp, sizeof(tmp));
		if (ret != ret_ok) {
			return ret_error;
		}

		http2d_buffer_add_char (&entry->ip_str_all, ',');
		http2d_buffer_add      (&entry->ip_str_all, tmp, strlen(tmp));

		addr = addr->ai_next;
	}

	return ret_ok;
}



/* Table
 */
ret_t
http2d_resolv_cache_init (http2d_resolv_cache_t *resolv)
{
	ret_t ret;

	ret = http2d_avl_init (&resolv->table);
	if (unlikely (ret != ret_ok)) return ret;

	ret = http2d_avl_set_case (&resolv->table, true);
	if (unlikely (ret != ret_ok)) return ret;

	HTTP2D_RWLOCK_INIT (&resolv->lock, NULL);
	return ret_ok;
}


ret_t
http2d_resolv_cache_mrproper (http2d_resolv_cache_t *resolv)
{
	http2d_avl_mrproper (AVL_GENERIC(&resolv->table), entry_free);
	HTTP2D_RWLOCK_DESTROY (&resolv->lock);

	return ret_ok;
}


ret_t
http2d_resolv_cache_get_default (http2d_resolv_cache_t **resolv)
{
	ret_t ret;

	if (unlikely (__global_resolv == NULL)) {
		HTTP2D_NEW_STRUCT (n, resolv_cache);

		ret = http2d_resolv_cache_init (n);
		if (ret != ret_ok) return ret;

		__global_resolv = n;
	}

	*resolv = __global_resolv;
	return ret_ok;
}


ret_t
http2d_resolv_cache_clean (http2d_resolv_cache_t *resolv)
{
	HTTP2D_RWLOCK_WRITER (&resolv->lock);
	http2d_avl_mrproper (AVL_GENERIC(&resolv->table), entry_free);
	HTTP2D_RWLOCK_UNLOCK (&resolv->lock);

	return ret_ok;
}


static ret_t
table_add_new_entry (http2d_resolv_cache_t        *resolv,
		     http2d_buffer_t              *domain,
		     http2d_resolv_cache_entry_t **entry)
{
	ret_t                          ret;
	http2d_resolv_cache_entry_t *n    = NULL;

	/* Instance the entry
	 */
	ret = entry_new (&n);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	/* Fill it up
	 */
	ret = entry_fill_up (n, domain);
	if (unlikely (ret != ret_ok)) {
		entry_free (n);
		return ret;
	}

	/* Add it to the table
	 */
	HTTP2D_RWLOCK_WRITER (&resolv->lock);
	ret = http2d_avl_add (&resolv->table, domain, (void **)n);
	HTTP2D_RWLOCK_UNLOCK (&resolv->lock);

	*entry = n;
	return ret_ok;
}


ret_t
http2d_resolv_cache_get_ipstr (http2d_resolv_cache_t  *resolv,
				 http2d_buffer_t        *domain,
				 const char              **ip)
{
	ret_t                          ret;
	http2d_resolv_cache_entry_t *entry = NULL;

	/* Look for the name in the cache
	 */
	HTTP2D_RWLOCK_READER (&resolv->lock);
	ret = http2d_avl_get (&resolv->table, domain, (void **)&entry);
	HTTP2D_RWLOCK_UNLOCK (&resolv->lock);

	if (ret != ret_ok) {
		TRACE (ENTRIES, "Resolve '%s': missed.\n", domain->buf);

		/* Bad luck: it wasn't cached
		 */
		ret = table_add_new_entry (resolv, domain, &entry);
		if (ret != ret_ok) {
			return ret;
		}
		TRACE (ENTRIES, "Resolve '%s': added succesfuly as '%s'.\n", domain->buf, entry->ip_str_all.buf);
	} else {
		TRACE (ENTRIES, "Resolve '%s': hit: %s\n", domain->buf, entry->ip_str_all.buf);
	}

	/* Return the ip string
	 */
	if (ip != NULL) {
		*ip = entry->ip_str.buf;
	}

	return ret_ok;
}


ret_t
http2d_resolv_cache_get_host (http2d_resolv_cache_t *resolv,
				http2d_buffer_t       *domain,
				void                    *sock_)
{
	ret_t                  ret;
	const struct addrinfo *addr = NULL;
	http2d_socket_t     *sock = sock_;

	/* Get addrinfo
	 */
	ret = http2d_resolv_cache_get_addrinfo (resolv, domain, &addr);
	if (ret != ret_ok) {
		return ret;
	}

	/* Copy it to the socket object
	 */
	ret = http2d_socket_update_from_addrinfo (sock, addr, 0);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}


ret_t
http2d_resolv_cache_get_addrinfo (http2d_resolv_cache_t *resolv,
				    http2d_buffer_t       *domain,
				    const struct addrinfo  **addr_info)
{
	ret_t                          ret;
	http2d_resolv_cache_entry_t *entry = NULL;

	/* Look for the name in the cache
	 */
	HTTP2D_RWLOCK_READER (&resolv->lock);
	ret = http2d_avl_get (&resolv->table, domain, (void **)&entry);
	HTTP2D_RWLOCK_UNLOCK (&resolv->lock);

	if (ret != ret_ok) {
		TRACE (ENTRIES, "Resolve '%s': missed.\n", domain->buf);

		/* Bad luck: it wasn't cached
		 */
		ret = table_add_new_entry (resolv, domain, &entry);
		if (ret != ret_ok) {
			TRACE (ENTRIES, "Resolve '%s': error ret=%d.\n", domain->buf, ret);
			return ret;
		}
		TRACE (ENTRIES, "Resolve '%s': added succesfuly as '%s'.\n", domain->buf, entry->ip_str.buf);
	} else {
		TRACE (ENTRIES, "Resolve '%s': hit.\n", domain->buf);
	}

	*addr_info = entry->addr;
	return ret_ok;
}
