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

#ifndef HTTP2D_SOCKET_SSL_H
#define HTTP2D_SOCKET_SSL_H

#include "socket.h"
#include "connection.h"

#define OPENSSL_LAST_ERROR(error)                                       \
        do { int n;                                                     \
                error = "unknown";                                      \
                while ((n = ERR_get_error()))                           \
                        error = ERR_error_string(n, NULL);              \
        } while (0)


#define OPENSSL_CLEAR_ERRORS                                            \
        do {                                                            \
                unsigned long openssl_error;                            \
                while ((openssl_error = ERR_get_error())) {             \
                        TRACE(ENTRIES, "Ignoring libssl error: %s\n",   \
                              ERR_error_string(openssl_error, NULL));   \
                }                                                       \
		errno = 0;						\
        } while(0)


ret_t _http2d_socket_ssl_init  (http2d_socket_t     *socket,
			        http2d_connection_t *conn,
			        int                 *wanted_io);

ret_t _http2d_socket_ssl_read  (http2d_socket_t     *socket,
			        const char          *buf,
			        int                  buf_size,
			        size_t              *pcnt_read,
			        int                 *wanted_io);

ret_t _http2d_socket_ssl_write (http2d_socket_t     *socket,
			        const char          *buf,
			        int                  buf_size,
			        size_t              *pcnt_written,
				int                 *wanted_io);

#endif /* HTTP2D_SOCKET_SSL_H */
