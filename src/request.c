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
#include "request.h"
#include "request-spdy.h"
#include "request-http.h"
#include "connection.h"


ret_t
http2d_request_init (http2d_request_t *req, void *conn)
{
	INIT_LIST_HEAD (&req->listed);

	req->conn       = conn;
	req->phase      = -1;
	req->error_code = http_internal_error;

	http2d_header_init (&req->header);

	return ret_ok;
}


ret_t
http2d_request_mrproper (http2d_request_t *req)
{
	http2d_header_mrproper (&req->header);
}


ret_t
http2d_request_new (http2d_request_t **req,
		    void              *conn)
{
	ret_t ret;
	HTTP2D_NEW_STRUCT (n,request);

	ret = http2d_request_init (n, conn);
	if (unlikely (ret != ret_ok)) {
		return ret;
	}

	*req = n;
	return ret_ok;
}


ret_t
http2d_request_free (http2d_request_t *req)
{
	http2d_request_mrproper (req);

	free (req);
	return ret_ok;
}


ret_t
http2d_request_step (http2d_request_t *req, int *wanted_io)
{
	ret_t                ret;
	http2d_connection_t *conn = CONN(req->conn);

	switch (conn->protocol.session) {
	case prot_session_SPDY:
		return http2d_request_spdy_step (&req->guts.spdy, wanted_io);
	default:
		return http2d_request_http_step (&req->guts.http, wanted_io);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
http2d_request_header_add_common (http2d_request_t *req)
{
	http2d_buffer_t key = HTTP2D_BUF_INIT_FAKE ("server");
	http2d_buffer_t val = HTTP2D_BUF_INIT_FAKE (PACKAGE_NAME);

	http2d_request_header_add (req, &key, &val);
	return ret_ok;
}


ret_t
http2d_request_header_add_response (http2d_request_t *req)
{
	http2d_connection_t *conn = CONN(req->conn);

	switch (conn->protocol.session) {
	case prot_session_SPDY:
		return http2d_request_spdy_header_add_response (&req->guts.spdy);
	default:
		return http2d_request_http_header_add_response (&req->guts.http);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


ret_t
http2d_request_header_add (http2d_request_t *req,
			   http2d_buffer_t  *key,
			   http2d_buffer_t  *val)
{
	http2d_connection_t *conn = CONN(req->conn);

	switch (conn->protocol.session) {
	case prot_session_SPDY:
		return http2d_request_spdy_header_add (&req->guts.spdy, key, val);
	default:
		return http2d_request_http_header_add (&req->guts.http, key, val);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}

ret_t
http2d_request_header_finish (http2d_request_t *req)
{
	http2d_connection_t *conn = CONN(req->conn);

	switch (conn->protocol.session) {
	case prot_session_SPDY:
		return http2d_request_spdy_header_finish (&req->guts.spdy);
	default:
		return http2d_request_http_header_finish (&req->guts.http);
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}
