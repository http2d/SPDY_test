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

#ifndef HTTP2D_REQUEST_HTTP_H
#define HTTP2D_REQUEST_HTTP_H

#include "common.h"
#include "buffer.h"
#include "request.h"

typedef struct {
	int bar;
} http2d_request_http_guts_t;


/* Request guts
 */
ret_t http2d_request_http_init                (http2d_request_http_guts_t *req_guts);
ret_t http2d_request_http_mrproper            (http2d_request_http_guts_t *req_guts);

ret_t http2d_request_http_step                (http2d_request_http_guts_t *req_guts,
					       int                        *wanted_io);
ret_t http2d_request_http_header_add          (http2d_request_http_guts_t *req_guts,
					       http2d_buffer_t            *key,
					       http2d_buffer_t            *val);

ret_t http2d_request_http_header_add_response (http2d_request_http_guts_t *req_guts);
ret_t http2d_request_http_header_finish       (http2d_request_http_guts_t *req_guts);
ret_t http2d_request_http_read_header         (http2d_request_http_guts_t *req_guts, int *wanted_io);




#endif /* HTTP2D_REQUEST_HTTP_H */
