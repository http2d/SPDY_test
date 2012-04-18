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

#ifndef HTTP2D_BIND_H
#define HTTP2D_BIND_H

#include "common-internal.h"
#include "list.h"
#include "buffer.h"
#include "config_node.h"
#include "socket.h"
#include "version.h"
#include "protocol.h"

typedef struct {
	http2d_list_t      listed;
	int                id;
	void              *srv;
	http2d_protocol_t  protocol;

	/* Events */
	ev_io              IO;

	/* Listener */
	http2d_socket_t    socket;

	/* Information */
	http2d_buffer_t    ip;
	int                port;

	/* Strings */
	http2d_buffer_t    server_string;
	http2d_buffer_t    server_string_w_port;

	http2d_buffer_t    server_address;
	http2d_buffer_t    server_port;

	/* Accepting */
	cuint_t            accept_continuous;
	cuint_t            accept_continuous_max;
	cuint_t            accept_recalculate;
} http2d_bind_t;

#define BIND(b)        ((http2d_bind_t *)(b))
#define BIND_IS_TLS(b) (BIND(b)->socket.is_tls == TLS)


ret_t http2d_bind_new  (http2d_bind_t **listener, void *srv);
ret_t http2d_bind_free (http2d_bind_t  *listener);

ret_t http2d_bind_configure   (http2d_bind_t *listener, http2d_config_node_t *config);
ret_t http2d_bind_set_default (http2d_bind_t *listener);
ret_t http2d_bind_accept_more (http2d_bind_t *listener, ret_t prev_ret);

ret_t http2d_bind_init_port   (http2d_bind_t         *listener,
			       cuint_t                listen_queue,
			       bool                   ipv6,
			       http2d_server_token_t  token);

#endif /* HTTP2D_BIND_H */
