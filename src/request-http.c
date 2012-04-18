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
#include "request-http.h"
#include "connection.h"


ret_t
http2d_request_http_init (http2d_request_http_guts_t *req_guts)
{
	return ret_ok;
}


ret_t
http2d_request_http_mrproper (http2d_request_http_guts_t *req_guts)
{
	return ret_ok;
}


ret_t
http2d_request_http_header_add (http2d_request_http_guts_t *http_guts,
				http2d_buffer_t            *key,
				http2d_buffer_t            *val)
{
	http2d_request_t    *req  = list_entry (http_guts, http2d_request_t, guts.http);
	http2d_connection_t *conn = CONN(req->conn);

	http2d_buffer_add_buffer (&conn->buffer_write, key);
	http2d_buffer_add_str    (&conn->buffer_write, ": ");
	http2d_buffer_add_buffer (&conn->buffer_write, val);

	return ret_ok;
}


ret_t
http2d_request_http_header_add_response (http2d_request_http_guts_t *http_guts)
{
	ret_t                ret;
	cuint_t              str_len;
	const char          *str      = NULL;
	http2d_request_t    *req      = list_entry (http_guts, http2d_request_t, guts.http);
	http2d_connection_t *conn     = CONN(req->conn);

	/* Status
	 */
	str_len = 0;
	str     = NULL;

	ret = http2d_http_code_to_string (req->error_code, &str, &str_len);
	if (unlikely ((ret != ret_ok) || (str == NULL) || (str_len == 0))) {
		return ret_error;
	}

	http2d_buffer_add      (&conn->buffer_write, str, str_len);
	http2d_buffer_add_char (&conn->buffer_write, ' ');

	/* version
	 */
	str_len = 0;
	str     = NULL;

	ret = http2d_http_version_to_string (conn->protocol.application, &str, &str_len);
	if (unlikely ((ret != ret_ok) || (str == NULL) || (str_len == 0))) {
		return ret_error;
	}

	http2d_buffer_add (&conn->buffer_write, str, str_len);
	return ret_ok;
}


ret_t
http2d_request_http_header_finish (http2d_request_http_guts_t *http_guts)
{
	http2d_request_t    *req  = list_entry (http_guts, http2d_request_t, guts.http);
	http2d_connection_t *conn = CONN(req->conn);

	http2d_buffer_add_str (&conn->buffer_write, CRLF_CRLF);
	return ret_ok;
}


ret_t
http2d_request_http_read_header (http2d_request_http_guts_t *req_guts,
				 int                        *wanted_io)
{
	// TODO
	return ret_ok;
}


ret_t
http2d_request_http_step (http2d_request_http_guts_t *req_guts,
			  int                        *wanted_io)
{
	return ret_ok;
}
