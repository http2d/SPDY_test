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
http2d_request_init_base (http2d_request_t *req, void *conn)
{
	ret_t ret;

	INIT_LIST_HEAD (&req->listed);

	ret = http2d_header_init (&req->header);
	if (unlikely (ret != ret_ok))
		return ret_error;

	req->conn       = conn;
	req->phase      = -1;
	req->error_code = http_internal_error;

	req->methods.free          = NULL;
	req->methods.step          = NULL;
	req->methods.header_base   = NULL;
	req->methods.header_add    = NULL;
	req->methods.header_finish = NULL;

	return ret_ok;
}


ret_t
http2d_request_mrproper (http2d_request_t *req)
{
	http2d_header_mrproper (&req->header);
}


void
http2d_request_free (http2d_request_t *req)
{
	if (unlikely (req->methods.free == NULL)) {
		SHOULDNT_HAPPEN;
		return;
	}

	req->methods.free (req);
}


ret_t
http2d_request_step (http2d_request_t *req,
		     int              *wanted_io)
{
	if (unlikely (req->methods.step == NULL)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return req->methods.step (req, wanted_io);
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
http2d_request_header_base (http2d_request_t *req)
{
	if (unlikely (req->methods.header_base == NULL)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return req->methods.header_base (req);
}

ret_t
http2d_request_header_add (http2d_request_t *req,
			   http2d_buffer_t  *key,
			   http2d_buffer_t  *val)
{
	if (unlikely (req->methods.header_add == NULL)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return req->methods.header_add (req, key, val);
}

ret_t
http2d_request_header_finish (http2d_request_t *req)
{
	if (unlikely (req->methods.header_finish == NULL)) {
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return req->methods.header_finish (req);
}


ret_t
_http2d_request_initialize (http2d_request_t *req)
{
	ret_t                ret;
	http2d_connection_t *conn = CONN(req->conn);

	// 1.- Match vserver
	// 2.- Match rule

	return ret_ok;
}
