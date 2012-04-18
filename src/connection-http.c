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
#include "connection-http.h"
#include "thread.h"
#include "socket-ssl.h"


static void  conn_http_free (http2d_connection_http_t *conn);
static ret_t conn_http_step (http2d_connection_http_t *conn);


ret_t
http2d_connection_http_new (http2d_connection_http_t **conn)
{
	ret_t ret;
	HTTP2D_NEW_STRUCT (n, connection_http);

	/* Base class */
	ret = http2d_connection_init_base (&n->base);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Properties */
	ret = http2d_request_init (&n->req, CONN(n));
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	/* Methods */
	n->base.methods.free = (http2d_conn_free) conn_http_free;
	n->base.methods.step = (http2d_conn_step) conn_http_step;

	*conn = n;
	return ret_ok;
}


static void
conn_http_free (http2d_connection_http_t *conn)
{
	/* Clean up base object */
	http2d_connection_mrproper (&conn->base);

	/* Properties */
	http2d_request_mrproper (&conn->req);

	/* Free the obj */
	free (conn);

}

static ret_t
conn_http_step (http2d_connection_http_t *conn)
{
	ret_t                ret;
	int                  wanted_io = EV_NONE;
	http2d_connection_t *conn_base = CONN_BASE(conn);
	http2d_thread_t     *thd       = THREAD(conn_base->thread);

	switch (conn_base->phase) {
	case phase_conn_http_handshake:
		if (conn_base->socket.is_tls == TLS) {
			ret = _http2d_socket_ssl_init (&conn_base->socket, conn_base, &wanted_io);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eof:
				return ret;
			case ret_eagain:
				if (wanted_io != 0) {
					http2d_connection_move_to_polling (conn_base, wanted_io);
				}
				return ret_ok;

			default:
				RET_UNKNOWN(ret);
				return ret_error;
			}
		}
		conn_base->phase = phase_conn_http_step;

	case phase_conn_http_step:
		ret = http2d_request_step (&conn->req, &wanted_io);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			if (wanted_io != 0) {
				http2d_connection_move_to_polling (conn_base, wanted_io);
			}
			return ret_ok;
		case ret_eof:
			conn_base->phase = phase_conn_http_shutdown;
			goto case_phase_conn_http_shutdown;
		default:
			RET_UNKNOWN(ret);
		}
		break;

	case phase_conn_http_shutdown:
	case_phase_conn_http_shutdown:
		ret = _http2d_connection_do_shutdown (conn_base);
		if (ret != ret_ok) {
			return ret;
		}
		conn_base->phase = phase_conn_http_shutdown;

	case phase_conn_http_lingering_read:
		wanted_io = -1;

		ret = _http2d_connection_do_linger_read (conn_base, &wanted_io);
		switch (ret) {
		case ret_eagain:
			if (wanted_io != -1) {
				http2d_connection_move_to_polling (conn_base, wanted_io);
				return ret_ok;
			}
			break;
		case ret_eof:
		case ret_error:
			http2d_thread_recycle_conn (thd, conn_base);
			return ret_eof;
		default:
			RET_UNKNOWN(ret);
			return ret_eof;
		}
		break;

	default:
		break;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}
