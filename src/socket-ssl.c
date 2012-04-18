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
#include "socket-ssl.h"
#include "server.h"
#include "virtual_server.h"
#include "error_log.h"
#include "util.h"

#define ENTRIES "socket,ssl"


static ret_t
socket_initialize (http2d_socket_t     *socket,
		   http2d_connection_t *conn)
{
	int                      re;
	const char              *error;
	http2d_virtual_server_t *vsrv_default = VSERVER(CONN_SRV(conn)->vservers.next);
	http2d_buffer_t          servername   = HTTP2D_BUF_INIT;

	/* Ensure the VServer support SSL
	 */
	if (vsrv_default->ssl_context == NULL) {
		return ret_not_found;
	}

	/* New session
	 */
	socket->session = SSL_new (vsrv_default->ssl_context);
	if (socket->session == NULL) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (HTTP2D_ERROR_SSL_CONNECTION, error);
		return ret_error;
	}

	/* Sets ssl to work in server mode
	 */
	SSL_set_accept_state (socket->session);

	/* Set the socket file descriptor
	 */
	re = SSL_set_fd (socket->session, socket->socket);
	if (re != 1) {
		OPENSSL_LAST_ERROR(error);
		LOG_ERROR (HTTP2D_ERROR_SSL_FD,
			   socket->socket, error);
		return ret_error;
	}

	/* Associate the connection to the SSL session
	 */
	SSL_set_app_data (socket->session, conn);

	/* Attempt to determine the vserver without SNI.
	 */
//	http2d_buffer_ensure_size(&servername, 40);
//	http2d_socket_ntop (&req->socket, servername.buf, servername.size);
//	http2d_cryptor_libssl_find_vserver (cryp->session, REQ_SRV(req), &servername, req);
//	http2d_buffer_mrproper (&servername);

	return ret_ok;
}


static ret_t
socket_handshake (http2d_socket_t *socket,
		  int             *wanted_io)
{
	int re;

	OPENSSL_CLEAR_ERRORS;

	re = SSL_do_handshake (socket->session);
	if (re == 0) {
		/* The TLS/SSL handshake was not successful but was
		 * shut down controlled and by the specifications of
		 * the TLS/SSL protocol.
		 */
		printf ("EOF\n");
		return ret_eof;

	} else if (re <= 0) {
		int         err;
		const char *error;
		int         err_sys = errno;

		err = SSL_get_error (socket->session, re);
		switch (err) {
		case SSL_ERROR_WANT_READ:
			*wanted_io = EV_READ;
			return ret_eagain;

		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
			*wanted_io = EV_WRITE;
			return ret_eagain;

		case SSL_ERROR_SYSCALL:
			if (err_sys == EAGAIN) {
				return ret_eagain;
			}
			return ret_error;

		case SSL_ERROR_SSL:
		case SSL_ERROR_ZERO_RETURN:
			return ret_error;
		default:
			OPENSSL_LAST_ERROR(error);
			LOG_ERROR (HTTP2D_ERROR_SSL_INIT, error);
			return ret_error;
		}
	}
	/* Report the connection details
	 */
#ifdef TRACE_ENABLED
	{
		HTTP2D_TEMP_VARS(buf, 256);
		const SSL_CIPHER *cipher = SSL_get_current_cipher (socket->session);

		if (cipher) {
			SSL_CIPHER_description (cipher, &buf[0], buf_size-1);

			TRACE (ENTRIES, "SSL: %s, %sREUSED, Ciphers: %s",
			       SSL_get_version(socket->session),
			       SSL_session_reused(socket->session)? "" : "Not ", &buf[0]);
		}
	}
#endif

	/* Disable Ciphers renegotiation (CVE-2009-3555)
	 */
	if (socket->session->s3) {
		socket->session->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
	}

	return ret_ok;
}


ret_t
_http2d_socket_ssl_init (http2d_socket_t     *socket,
			 http2d_connection_t *conn,
			 int                 *wanted_io)
{
	ret_t ret;

	switch (socket->phase_init) {
	case phase_init_set_up:
		ret = socket_initialize (socket, conn);
		printf ("socket_initialize %d\n", ret);
		if (unlikely (ret != ret_ok)) {
			return ret;
		}

		socket->phase_init = phase_init_handshake;

	case phase_init_handshake:
		ret = socket_handshake (socket, wanted_io);
		printf ("socket_handshake %d\n", ret);
		return ret;

	default:
		break;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
_http2d_socket_ssl_read (http2d_socket_t *socket,
			 const char      *buf,
			 int              buf_size,
			 size_t          *pcnt_read,
			 int             *wanted_io)
{
	int     re;
	int     error;
	ssize_t len;

	OPENSSL_CLEAR_ERRORS;

	len = SSL_read (socket->session, (char *)buf, buf_size);
	if (likely (len > 0)) {
		*pcnt_read = len;
//		if (SSL_pending (socket->session))
//			return ret_eagain;
		return ret_ok;
	}

	if (len == 0) {
		return ret_eof;
	}

	/* len < 0 */
	error = errno;
	re = SSL_get_error (socket->session, len);
	switch (re) {
	case SSL_ERROR_WANT_READ:
		*wanted_io = EV_READ;
		return ret_eagain;
	case SSL_ERROR_WANT_WRITE:
		*wanted_io = EV_WRITE;
		return ret_eagain;
	case SSL_ERROR_ZERO_RETURN:
		return ret_eof;
	case SSL_ERROR_SSL:
		return ret_error;
	case SSL_ERROR_SYSCALL:
		switch (error) {
		case EAGAIN:
			return ret_eagain;
		case EPIPE:
		case ECONNRESET:
			return ret_eof;
		default:
			LOG_ERRNO_S (error, http2d_err_error,
				     HTTP2D_ERROR_SSL_SR_DEFAULT);
		}
		return ret_error;
	}

	LOG_ERROR (HTTP2D_ERROR_SSL_SR_ERROR,
		   SSL_get_fd(socket->session), (int)len, ERR_error_string(re, NULL));
	return ret_error;
}


ret_t
_http2d_socket_ssl_write (http2d_socket_t *socket,
			  const char      *buf,
			  int              buf_len,
			  size_t          *pcnt_written,
			  int             *wanted_io)
{
	int     re;
	ssize_t len;
	int     error;

	OPENSSL_CLEAR_ERRORS;

	len = SSL_write (socket->session, buf, buf_len);
	if (likely (len > 0) ) {
		if (len != buf_len) {
			SHOULDNT_HAPPEN;
			return ret_error;
		}

		TRACE (ENTRIES, "SSL-Write. Buffer sent: %p (total len %d)\n", buf, buf_len);
		*pcnt_written = buf_len;
		return ret_ok;
	}

	if (len == 0) {
		TRACE (ENTRIES",write", "write got %s\n", "EOF");
		return ret_eof;
	}

	/* len < 0 */
	error = errno;

	re = SSL_get_error (socket->session, len);
	switch (re) {
	case SSL_ERROR_WANT_READ:
		TRACE (ENTRIES",write", "write len=%d (written=0), EAGAIN(EV_READ)\n", buf_len);
		*wanted_io = EV_READ;
		return ret_eagain;

	case SSL_ERROR_WANT_WRITE:
		TRACE (ENTRIES",write", "write len=%d (written=0), EAGAIN(EV_WRITE)\n", buf_len);
		*wanted_io = EV_WRITE;
		return ret_eagain;

	case SSL_ERROR_SYSCALL:
		switch (error) {
		case EAGAIN:
			TRACE (ENTRIES",write", "write len=%d (written=0), EAGAIN\n", buf_len);
			return ret_eagain;
#ifdef ENOTCONN
		case ENOTCONN:
#endif
		case EPIPE:
		case ECONNRESET:
			TRACE (ENTRIES",write", "write len=%d (written=0), EOF\n", buf_len);
			return ret_eof;
		default:
			LOG_ERRNO_S (error, http2d_err_error,
				     HTTP2D_ERROR_SSL_SW_DEFAULT);
		}

		TRACE (ENTRIES",write", "write len=%d (written=0), ERROR: %s\n", buf_len, ERR_error_string(re, NULL));
		return ret_error;

	case SSL_ERROR_SSL:
		TRACE (ENTRIES",write", "write len=%d (written=0), ERROR: %s\n", buf_len, ERR_error_string(re, NULL));
		return ret_error;
	}

	LOG_ERROR (HTTP2D_ERROR_SSL_SW_ERROR,
		   SSL_get_fd(socket->session), (int)len, ERR_error_string(re, NULL));

	TRACE (ENTRIES",write", "write len=%d (written=0), ERROR: %s\n", buf_len, ERR_error_string(re, NULL));
	return ret_error;

}
