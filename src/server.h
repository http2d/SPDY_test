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

#ifndef HTTP2D_SERVER_H
#define HTTP2D_SERVER_H

#include "common.h"
#include "list.h"
#include "buffer.h"
#include "config_node.h"
#include "virtual_server.h"
#include "connection.h"
#include <ev.h>


typedef struct {
	struct ev_loop       *main_loop;
	http2d_list_t         binds;
	http2d_list_t         threads;
	http2d_list_t         vservers;
	http2d_config_node_t  config;
	http2d_buffer_t       alternate_protocol;
} http2d_server_t;

#define SRV(s) ((http2d_server_t *)(s))

ret_t http2d_server_init        (http2d_server_t *srv);
ret_t http2d_server_mrproper    (http2d_server_t *srv);

ret_t http2d_server_read_conf   (http2d_server_t *srv, http2d_buffer_t *buf);
ret_t http2d_server_initialize  (http2d_server_t *srv);
ret_t http2d_server_run         (http2d_server_t *srv);

ret_t http2d_server_get_vserver (http2d_server_t          *srv,
				 http2d_buffer_t          *name,
				 http2d_connection_t      *conn,
				 http2d_virtual_server_t **vsrv);

#endif /* HTTP2D_SERVER_H */
