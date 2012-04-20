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
#include "connection.h"
#include "thread.h"
#include "util.h"
#include "request-spdy.h"
#include "request-http.h"

#define ENTRIES "conn"


ret_t
http2d_connection_init_base (http2d_connection_t *conn)
{
	ret_t ret;

	INIT_LIST_HEAD (&conn->listed);
	INIT_LIST_HEAD (&conn->requests);

	ret = http2d_socket_init (&conn->socket);
	if (unlikely (ret != ret_ok))
		return ret_error;

	conn->thread = NULL;
	conn->bind   = NULL;
	conn->srv    = NULL;
	conn->phase  = 0;

	conn->methods.free = NULL;
	conn->methods.step = NULL;

	http2d_protocol_init (&conn->protocol);
	http2d_buffer_init (&conn->buffer_read);
	http2d_buffer_init (&conn->buffer_write);
	http2d_buffer_init (&conn->buffer_write_ssl);

	return ret_ok;
}


void
http2d_connection_free (http2d_connection_t *conn)
{
	if (unlikely (conn->methods.free == NULL)) {
		SHOULDNT_HAPPEN;
		return;
	}

	conn->methods.free (conn);
}


ret_t
http2d_connection_step (http2d_connection_t *conn)
{
	if (unlikely (conn->methods.step == NULL)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return conn->methods.step (conn);
}


ret_t
http2d_connection_mrproper (http2d_connection_t *conn)
{
	http2d_list_t *i, *j;

	http2d_socket_close (&conn->socket);
	http2d_socket_mrproper (&conn->socket);

	http2d_buffer_mrproper (&conn->buffer_read);
	http2d_buffer_mrproper (&conn->buffer_write);
	http2d_buffer_mrproper (&conn->buffer_write_ssl);

	list_for_each_safe (i, j, &conn->requests) {
		http2d_request_free (REQ(i));
	}

	if (THREAD(conn->thread) != NULL) {
		ev_io_stop (THREAD(conn->thread)->loop, &conn->IO);
	}

	return ret_ok;
}


static void
io_cb (struct ev_loop *loop, ev_io *w, int revents)
{
	ret_t                ret;
	http2d_connection_t *conn = list_entry (w, http2d_connection_t, IO);

	if ((revents & EV_READ) ||
	    (revents & EV_WRITE))
	{
		http2d_connection_move_to_active (conn);
		return;
	}

	SHOULDNT_HAPPEN;
}


ret_t
http2d_connection_move_to_polling (http2d_connection_t *conn, int mode)
{
	http2d_thread_t *thd = THREAD(conn->thread);

	http2d_list_del (&conn->listed);
	http2d_list_add (&conn->listed, &thd->conns.polling);

	ev_io_init (&conn->IO, io_cb, conn->socket.socket, mode);
	ev_io_start (thd->loop, &conn->IO);

	return ret_ok;
}


ret_t
http2d_connection_move_to_active (http2d_connection_t *conn)
{
	http2d_thread_t *thd = THREAD(conn->thread);

	ev_io_stop (thd->loop, &conn->IO);

	http2d_list_del (&conn->listed);
	http2d_list_add (&conn->listed, &thd->conns.active);

	return ret_ok;
}



/* Internal utility functions
 */

ret_t
_http2d_connection_do_read (http2d_connection_t *conn,
			    int                 *wanted_io)
{
	ret_t  ret;
	size_t read = 0;

	ret = http2d_socket_read_buf (&conn->socket, &conn->buffer_read, 16382, &read, wanted_io);
	switch (ret) {
	case ret_ok:
	case ret_eof:
	case ret_eagain:
		return ret;
	default:
		RET_UNKNOWN(ret);
	}

	return ret_ok;
}


ret_t
_http2d_connection_do_write (http2d_connection_t *conn, int *wanted_io)
{
	ret_t            ret;
	size_t           written = 0;
	bool             ssl     = (conn->socket.is_tls == TLS);
	http2d_buffer_t *buf     = (ssl) ? &conn->buffer_write_ssl : &conn->buffer_write;

	printf ("conn->buffer_write %d\n", conn->buffer_write.len);
	printf ("conn->buffer_write_ssl %d\n", conn->buffer_write_ssl.len);

	/* SSL
	 */
	if (ssl) {
		/* We keep two 'write' buffers. 'buffer_write' is
		 * where new data is written by the server core.
		 * 'buffer_write_ssl' what we pass to SSL_write().
		 */
		if ((! http2d_buffer_is_empty (&conn->buffer_write)) &&
		    (http2d_buffer_is_empty (&conn->buffer_write_ssl)))
		{
			printf ("swap buffers\n");
			http2d_buffer_swap_buffers (&conn->buffer_write_ssl, &conn->buffer_write);
		}
	}

	/* Write
	 */
	ret = http2d_socket_write_buf (&conn->socket, buf, &written, wanted_io);

	/* Free the sent portion
	 */
	if (written > 0) {
		http2d_buffer_move_to_begin (buf, written);
	}

	/* ret */
	switch (ret) {
	case ret_ok:
	case ret_eof:
	case ret_eagain:
		return ret;
	default:
		RET_UNKNOWN(ret);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
_http2d_connection_new_req (http2d_connection_t  *conn,
			    http2d_request_t    **req)
{
	ret_t             ret;
	http2d_request_t *new_req;

	/* New request obj */
	if (conn->protocol.session == prot_session_SPDY) {
		http2d_request_spdy_t *n = NULL;

		ret = http2d_request_spdy_new (&n, conn);
		new_req = REQ(n);
	}
	else {
		http2d_request_http_t *n = NULL;

		ret = http2d_request_http_new (&n, conn);
		new_req = REQ(n);
	}

	if (ret != ret_ok) {
		return ret_error;
	}

	http2d_list_add (&new_req->listed, &conn->requests);

	*req = new_req;
	return ret_ok;
}


ret_t
_http2d_connection_do_shutdown (http2d_connection_t *conn)
{
	ret_t ret;

	/* Do not use SSL for the lingering read phase
	 */
	conn->socket.is_tls         = non_TLS;
	conn->protocol.presentation = prot_presentation_none;

        /* Shut down the socket for write, which will send a FIN to
         * the peer. If shutdown fails then the socket is unusable.
         */
        ret = http2d_socket_shutdown (&conn->socket, SHUT_WR);
        if (unlikely (ret != ret_ok)) {
                TRACE (ENTRIES, "Could not shutdown (%d, SHUT_WR)\n", conn->socket.socket);
                return ret_error;
        }

        TRACE (ENTRIES, "Shutdown (%d, SHUT_WR): successful\n", conn->socket.socket);
        return ret_ok;
}


ret_t
_http2d_connection_do_linger_read (http2d_connection_t *conn, int *wanted_io)
{
        ret_t            ret;
	char             tmp[512];
        size_t           cnt_read   = 0;
        int              retries    = 2;
        http2d_thread_t *thread     = THREAD(conn->thread);

        TRACE(ENTRIES",linger", "Linger read, socket is %s\n",
	      conn->socket.closed ? "already closed" : "still open");

        while (true) {
                /* Read from the socket to nowhere
                 */
                ret = http2d_socket_read (&conn->socket, tmp, sizeof(tmp) - 1, &cnt_read, wanted_io);

                switch (ret) {
                case ret_eof:
                        TRACE (ENTRIES",linger", "%s\n", "EOF");
                        return ret_eof;
                case ret_error:
                        TRACE (ENTRIES",linger", "%s\n", "error");
                        return ret_error;
                case ret_eagain:
                        TRACE (ENTRIES",linger", "read %u, eagain\n", cnt_read);
                        return ret_eagain;
                case ret_ok:
                        TRACE (ENTRIES",linger", "%u bytes tossed away\n", cnt_read);

                        retries--;
                        if ((cnt_read >= sizeof(tmp) - 2) && (retries > 0))
                                continue;

                        return ret_eof;
                default:
                        RET_UNKNOWN(ret);
                        return ret_error;
                }
                /* NOTREACHED */
        }
}
