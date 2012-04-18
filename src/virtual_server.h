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

#ifndef HTTP2D_VIRTUAL_SERVER_H
#define HTTP2D_VIRTUAL_SERVER_H

#include "common.h"
#include "buffer.h"
#include "socket.h"
#include "config_node.h"

typedef struct {
	http2d_buffer_t  name;
	int              priority;
	void            *srv;

	/* SSL */
	http2d_buffer_t  ssl_certificate_file;
	http2d_buffer_t  ssl_certificate_key_file;
	http2d_buffer_t  ssl_ca_list_file;
	http2d_buffer_t  ssl_ciphers;
	bool             ssl_cipher_server_preference;

	SSL_CTX         *ssl_context;
} http2d_virtual_server_t;

#define VSERVER(v) ((http2d_virtual_server_t *)(v))


ret_t http2d_virtual_server_new        (http2d_virtual_server_t **vsrv, void *srv);
ret_t http2d_virtual_server_free       (http2d_virtual_server_t  *vsrv);
ret_t http2d_virtual_server_initialize (http2d_virtual_server_t  *vsrv);

#endif /* HTTP2D_VIRTUAL_SERVER_H */
