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

#ifndef HTTP2D_REQUEST_H
#define HTTP2D_REQUEST_H

#include "common.h"
#include "buffer.h"
#include "list.h"
#include "header.h"

typedef void  (* http2d_req_free)          (void *req);
typedef ret_t (* http2d_req_step)          (void *req, int *wanted_io);
typedef ret_t (* http2d_req_header_base)   (void *req);
typedef ret_t (* http2d_req_header_add)    (void *req, http2d_buffer_t *key, http2d_buffer_t *val);
typedef ret_t (* http2d_req_header_finish) (void *req);


typedef struct {
	http2d_list_t           listed;
	http2d_header_t         header;
	void                   *conn;
	int                     phase;

	/* Response */
	http2d_http_t           error_code;

	/* Virtual methods */
	struct {
		http2d_req_free          free;
		http2d_req_step          step;
		http2d_req_header_base   header_base;
		http2d_req_header_add    header_add;
		http2d_req_header_finish header_finish;
	} methods;

} http2d_request_t;

#define REQ(r) ((http2d_request_t *)(r))


ret_t http2d_request_init_base (http2d_request_t *req, void *conn);
ret_t http2d_request_mrproper  (http2d_request_t *req);

void  http2d_request_free     (http2d_request_t  *req);
ret_t http2d_request_init     (http2d_request_t  *req, void *conn);
ret_t http2d_request_mrproper (http2d_request_t  *req);

ret_t http2d_request_step                (http2d_request_t *req, int *wanted_io);
ret_t http2d_request_header_add_response (http2d_request_t *req);
ret_t http2d_request_header_add_common   (http2d_request_t *req);
ret_t http2d_request_header_add          (http2d_request_t *req, http2d_buffer_t *key, http2d_buffer_t *val);
ret_t http2d_request_header_finish       (http2d_request_t *req);

ret_t _http2d_request_initialize         (http2d_request_t *req);


#endif /* HTTP2D_REQUEST_H */
