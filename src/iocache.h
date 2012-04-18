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

#ifndef HTTP2D_IOCACHE_H
#define HTTP2D_IOCACHE_H

#include "common.h"
#include "cache.h"
#include "server.h"
#include "config_node.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
	http2d_cache_t        cache;

	/* Limits */
	cuint_t                 max_file_size;
	cuint_t                 min_file_size;
	cuint_t                 lasting_mmap;
	cuint_t                 lasting_stat;
} http2d_iocache_t;

typedef enum {
	iocache_nothing = 0,
	iocache_stat    = 1,
	iocache_mmap    = 1 << 1
} http2d_iocache_info_t;

typedef struct {
	/* Inheritance */
	http2d_cache_entry_t  base;
	http2d_iocache_info_t info;

	/* Information to cache */
	struct stat             state;
	ret_t                   state_ret;
	void                   *mmaped;
	size_t                  mmaped_len;
} http2d_iocache_entry_t;

#define IOCACHE(x)       ((http2d_iocache_t *)(x))
#define IOCACHE_ENTRY(x) ((http2d_iocache_entry_t *)(x))


/* I/O cache
 */
ret_t http2d_iocache_new             (http2d_iocache_t **iocache);
ret_t http2d_iocache_free            (http2d_iocache_t  *iocache);

ret_t http2d_iocache_init            (http2d_iocache_t  *iocache);
ret_t http2d_iocache_mrproper        (http2d_iocache_t  *iocache);

ret_t http2d_iocache_configure       (http2d_iocache_t     *iocache,
					http2d_config_node_t *conf);

/* I/O cache entry
 */
ret_t http2d_iocache_entry_unref     (http2d_iocache_entry_t **entry);

/* Autoget: Get or Update
 */
ret_t http2d_iocache_autoget         (http2d_iocache_t        *iocache,
					http2d_buffer_t         *file,
					http2d_iocache_info_t    info,
					http2d_iocache_entry_t **ret_io);

ret_t http2d_iocache_autoget_fd      (http2d_iocache_t        *iocache,
					http2d_buffer_t         *file,
					http2d_iocache_info_t    info,
					int                       *fd,
					http2d_iocache_entry_t **ret_io);

/* Misc */
ret_t http2d_iocache_get_mmaped_size (http2d_iocache_t *iocache, size_t *total);

#endif /* HTTP2D_IOCACHE_H */
