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

#ifndef HTTP2D_CONNECTION_H
#define HTTP2D_CONNECTION_H

#include "common.h"
#include "protocol.h"
#include "socket.h"
#include "list.h"
#include "bind.h"
#include "connection-spdy.h"
#include "connection-http.h"
#include "request.h"
#include <ev.h>


typedef enum {
	conn_close_open,
	conn_close_shutdown,
	conn_close_linger_read,
	conn_close_closed
} http2d_conn_close_phase_t;

typedef struct {
	http2d_list_t             listed;
	http2d_socket_t           socket;
	ev_io                     IO;
	http2d_protocol_t         protocol;
	http2d_buffer_t           buffer_read;
	http2d_buffer_t           buffer_write;
	http2d_buffer_t           buffer_write_ssl;
	int                       phase;
	http2d_list_t             requests;
	http2d_conn_close_phase_t close_phase;

	/* References */
	void                     *thread;
	void                     *bind;
	void                     *srv;

	/* Protocol specific */
	union {
		http2d_connection_spdy_guts_t spdy;
		http2d_connection_http_guts_t http;
	} guts;

} http2d_connection_t;

#define CONN(c)     ((http2d_connection_t *)(c))
#define CONN_SRV(c) SRV(CONN(c)->srv)

ret_t http2d_connection_new  (http2d_connection_t **conn);
ret_t http2d_connection_free (http2d_connection_t  *conn);
ret_t http2d_connection_step (http2d_connection_t  *conn);

ret_t http2d_connection_move_to_active  (http2d_connection_t *conn);
ret_t http2d_connection_move_to_polling (http2d_connection_t *conn, int mode);

/* Protocol specfic */
ret_t http2d_connection_spdy_step (http2d_connection_t *conn);
ret_t http2d_connection_http_step (http2d_connection_t *conn);

/* Intenal utility functions */
ret_t _http2d_connection_new_req        (http2d_connection_t *conn, http2d_request_t **req);
ret_t _http2d_connection_do_read        (http2d_connection_t *conn, int *wanted_io);
ret_t _http2d_connection_do_write       (http2d_connection_t *conn, int *wanted_io);
ret_t _http2d_connection_do_shutdown    (http2d_connection_t *conn);
ret_t _http2d_connection_do_linger_read (http2d_connection_t *conn, int *wanted_io);

#endif /* HTTP2D_CONNECTION_H */
