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

#ifndef HTTP2D_REQUEST_SPDY_H
#define HTTP2D_REQUEST_SPDY_H

#include "buffer.h"

typedef enum {
	phase_spdy_nothing,
	phase_spdy_process_header,
	phase_spdy_setup_request,
	phase_spdy_init,
	phase_spdy_reading_post,
	phase_spdy_add_headers,
	phase_spdy_stepping,
} http2d_request_spdy_phase_t;

typedef struct {
	http2d_buffer_t header;
	int32_t         header_num;
} http2d_request_spdy_guts_t;

#define SPDY_GUTS_GET_REQ(r) (list_entry ((r), http2d_request_t, guts.spdy))


ret_t http2d_request_spdy_init                (http2d_request_spdy_guts_t *req_guts);
ret_t http2d_request_spdy_mrproper            (http2d_request_spdy_guts_t *req_guts);

ret_t http2d_request_spdy_step                (http2d_request_spdy_guts_t *req_guts,
					       int                        *wanted_io);
ret_t http2d_request_spdy_header_add          (http2d_request_spdy_guts_t *req_guts,
					       http2d_buffer_t            *key,
					       http2d_buffer_t            *val);

ret_t http2d_request_spdy_header_add_response (http2d_request_spdy_guts_t *req_guts);
ret_t http2d_request_spdy_header_finish       (http2d_request_spdy_guts_t *req_guts);

#endif /* HTTP2D_REQUEST_SPDY_H */
