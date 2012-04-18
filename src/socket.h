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

#ifndef HTTP2D_SOCKET_H
#define HTTP2D_SOCKET_H

#include "common-internal.h"
#include "socket_lowlevel.h"
#include "buffer.h"

#include <ev.h>

#include <openssl/lhash.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>

/* Socket status
 */

// TODO: Get rid of this type
typedef enum {
	socket_reading = EV_READ,
	socket_writing = EV_WRITE,
	socket_closed  = EV_NONE,
} http2d_socket_status_t;


/* Socket crypt
 */
typedef enum {
	non_TLS,
	TLS
} http2d_socket_type_t;

/* Phases
 */
typedef enum {
	phase_init_set_up,
	phase_init_handshake,
} http2d_socket_init_phase_t;


/* Socket address
 */
typedef union {
	struct sockaddr         sa;
	struct sockaddr_in      sa_in;

#ifdef HAVE_SOCKADDR_UN
	struct sockaddr_un      sa_un;
#endif
#ifdef HAVE_SOCKADDR_IN6
	struct sockaddr_in6     sa_in6;
#endif
#ifdef HAVE_SOCKADDR_STORAGE
	struct sockaddr_storage sa_stor;
#endif
} http2d_sockaddr_t;


/* Socket
 */
typedef struct {
	int                         socket;
	http2d_sockaddr_t           client_addr;
	socklen_t                   client_addr_len;
	bool                        closed;
	http2d_socket_type_t        is_tls;

	/* SSL support */
	SSL                        *session;
	SSL_CTX                    *ssl_ctx;
	http2d_socket_init_phase_t  phase_init;
} http2d_socket_t;


#define S_SOCKET(s)            ((http2d_socket_t)(s))
#define S_SOCKET_FD(s)         ((s).socket)

#define SOCKET(s)              ((http2d_socket_t *)(s))
#define SOCKET_FD(s)           (SOCKET(s)->socket)
#define SOCKET_AF(s)           (SOCKET(s)->client_addr.sa.sa_family)

#define SOCKET_ADDR(s)         (SOCKET(s)->client_addr)
#define SOCKET_ADDR_UNIX(s)    ((struct sockaddr_un  *) &SOCKET_ADDR(s))
#define SOCKET_ADDR_IPv4(s)    ((struct sockaddr_in  *) &SOCKET_ADDR(s))
#define SOCKET_ADDR_IPv6(s)    ((struct sockaddr_in6 *) &SOCKET_ADDR(s))

#define SOCKET_SIN_PORT(s)     (SOCKET(s)->client_addr.sa_in.sin_port)
#define SOCKET_SIN_ADDR(s)     (SOCKET(s)->client_addr.sa_in.sin_addr)
#define SOCKET_SIN6_ADDR(s)    (SOCKET(s)->client_addr.sa_in6.sin6_addr)

#define SOCKET_SUN_PATH(s)     (SOCKET(s)->client_addr.sa_un.sun_path)

#define SOCKET_ADDRESS_IPv4(s) (SOCKET_ADDR_IPv4(s)->sin_addr.s_addr)
#define SOCKET_ADDRESS_IPv6(s) (SOCKET_ADDR_IPv6(s)->sin6_addr.s6_addr)


#define http2d_socket_configured(c)    (SOCKET_FD(c) >= 0)
#define http2d_socket_is_connected(c)  (http2d_socket_configured(c) && (!SOCKET(c)->closed))

ret_t http2d_socket_init              (http2d_socket_t *socket);
ret_t http2d_socket_mrproper          (http2d_socket_t *socket);
ret_t http2d_socket_clean             (http2d_socket_t *socket);

//ret_t http2d_socket_init_tls          (http2d_socket_t *socket, http2d_virtual_server_t *vserver, http2d_request_t *conn, http2d_socket_status_t *blocking);
ret_t http2d_socket_init_client_tls   (http2d_socket_t *socket, http2d_buffer_t *host);

ret_t http2d_socket_close             (http2d_socket_t *socket);
ret_t http2d_socket_shutdown          (http2d_socket_t *socket, int how);
ret_t http2d_socket_reset             (http2d_socket_t *socket);
ret_t http2d_socket_accept            (http2d_socket_t *socket, http2d_socket_t *server_socket);
ret_t http2d_socket_accept_fd         (http2d_socket_t *socket, int *new_fd, http2d_sockaddr_t *sa);
ret_t http2d_socket_flush             (http2d_socket_t *socket);
ret_t http2d_socket_test_read         (http2d_socket_t *socket);

ret_t http2d_socket_create_fd         (http2d_socket_t *socket, unsigned short int family);
ret_t http2d_socket_bind              (http2d_socket_t *socket, int port, http2d_buffer_t *listen_to);
ret_t http2d_socket_listen            (http2d_socket_t *socket, int backlog);

ret_t http2d_socket_write_buf         (http2d_socket_t *socket, http2d_buffer_t *buf, size_t *written, int *wanted_io);
ret_t http2d_socket_read_buf          (http2d_socket_t *socket, http2d_buffer_t *buf, size_t count, size_t *read, int *wanted_io);
ret_t http2d_socket_sendfile          (http2d_socket_t *socket, int fd, size_t size, off_t *offset, ssize_t *sent);
ret_t http2d_socket_connect           (http2d_socket_t *socket);

ret_t http2d_socket_ntop              (http2d_socket_t *socket, char *buf, size_t buf_size);
ret_t http2d_socket_pton              (http2d_socket_t *socket, http2d_buffer_t *buf);
ret_t http2d_socket_gethostbyname     (http2d_socket_t *socket, http2d_buffer_t *hostname);
ret_t http2d_socket_set_cork          (http2d_socket_t *socket, bool enable);


/* Low level functions
 */
ret_t http2d_socket_read   (http2d_socket_t *socket, char *buf, int buf_size, size_t *pcnt_read, int *wanted_io);
ret_t http2d_socket_write  (http2d_socket_t *socket, const char *buf, int len, size_t *pcnt_written, int *wanted_io);
ret_t http2d_socket_writev (http2d_socket_t *socket, const struct iovec *vector, uint16_t vector_len, size_t *pcnt_written, int *wanted_io);

/* Extra
 */
ret_t http2d_socket_set_sockaddr         (http2d_socket_t *socket, int fd, http2d_sockaddr_t *sa);
ret_t http2d_socket_update_from_addrinfo (http2d_socket_t *socket, const struct addrinfo *addr_info, cuint_t num);

#endif /* HTTP2D_SOCKET_H */
