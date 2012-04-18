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

#ifndef HTTP2D_THREAD_H
#define HTTP2D_THREAD_H

#include "common.h"
#include "list.h"
#include "connection.h"
#include <ev.h>

typedef struct {
	http2d_list_t   listed;
	pthread_t       thread;
	struct ev_loop *loop;
	void           *srv;

	struct {
		http2d_list_t   active;
		http2d_list_t   polling;
	} conns;

	struct {
		struct ev_async signal;
		pthread_mutex_t mutex;
		http2d_list_t   list;
	} new_conns;

} http2d_thread_t;

#define THREAD(t) ((http2d_thread_t *)(t))

ret_t http2d_thread_new          (http2d_thread_t **thd, void *srv);
ret_t http2d_thread_free         (http2d_thread_t  *thd);
ret_t http2d_thread_add_conn     (http2d_thread_t  *thd, http2d_connection_t *conn);
ret_t http2d_thread_recycle_conn (http2d_thread_t  *thd, http2d_connection_t *conn);

#endif /* HTTP2D_THREAD_H */
