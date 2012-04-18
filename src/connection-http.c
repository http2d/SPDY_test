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


ret_t
http2d_connection_http_step (http2d_connection_t *conn)
{
	ret_t            ret;
	int              wanted_io = EV_NONE;
	http2d_thread_t *thd       = THREAD(conn->thread);

	switch (conn->phase) {
	case phase_conn_http_handshake:
		if (conn->socket.is_tls == TLS) {
			ret = _http2d_socket_ssl_init (&conn->socket, conn, &wanted_io);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eof:
				return ret;
			case ret_eagain:
				if (wanted_io != 0) {
					http2d_connection_move_to_polling (conn, wanted_io);
				}
				return ret_ok;

			default:
				RET_UNKNOWN(ret);
				return ret_error;
			}
		}
		conn->phase = phase_conn_http_step;

	case phase_conn_http_step:
		ret = http2d_request_step (&conn->guts.http.req, &wanted_io);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			if (wanted_io != 0) {
				http2d_connection_move_to_polling (conn, wanted_io);
			}
			return ret_ok;
		case ret_eof:
			conn->phase = phase_conn_http_shutdown;
			goto case_phase_conn_http_shutdown;
		default:
			RET_UNKNOWN(ret);
		}
		break;

	case phase_conn_http_shutdown:
	case_phase_conn_http_shutdown:
		ret = _http2d_connection_do_shutdown (conn);
		if (ret != ret_ok) {
			return ret;
		}
		conn->phase = phase_conn_http_shutdown;

	case phase_conn_http_lingering_read:
		wanted_io = -1;

		ret = _http2d_connection_do_linger_read (conn, &wanted_io);
		switch (ret) {
		case ret_eagain:
			if (wanted_io != -1) {
				http2d_connection_move_to_polling (conn, wanted_io);
				return ret_ok;
			}
			break;
		case ret_eof:
		case ret_error:
			http2d_thread_recycle_conn (thd, conn);
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


ret_t
http2d_connection_http_guts_init (http2d_connection_http_guts_t *http_guts)
{
	ret_t                ret;
	http2d_connection_t *conn = list_entry (http_guts, http2d_connection_t, guts.http);

	ret = http2d_request_init (&http_guts->req, conn);
	if (ret != ret_ok) {
		return ret;
	}

	return ret_ok;
}


ret_t
http2d_connection_http_guts_mrproper (http2d_connection_http_guts_t *http_guts)
{
	http2d_request_mrproper (&http_guts->req);
	return ret_ok;
}
