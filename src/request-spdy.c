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
#include "connection.h"
#include "buffer.h"
#include "spdy.h"
#include "spdy-zlib.h"
#include "connection-spdy.h"

static ret_t req_spdy_header_add    (http2d_request_spdy_t *req_spdy, http2d_buffer_t *key, http2d_buffer_t *val);
static ret_t req_spdy_header_base   (http2d_request_spdy_t *req_spdy);
static ret_t req_spdy_header_finish (http2d_request_spdy_t *req_spdy);
static ret_t req_spdy_step          (http2d_request_spdy_t *req_spdy, int *wanted_io);
static void  req_spdy_free          (http2d_request_spdy_t *req_spdy);


ret_t
http2d_request_spdy_new (http2d_request_spdy_t **req_spdy, void *conn)
{
	ret_t ret;
	HTTP2D_NEW_STRUCT (n, request_spdy);

	/* Base class */
	ret = http2d_request_init_base (&n->base, conn);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Properties */
	http2d_buffer_init (&n->header);

	n->header_num = 0;
	n->base.phase = phase_spdy_nothing;

	/* Methods */
	n->base.methods.free          = (http2d_req_free) req_spdy_free;
	n->base.methods.step          = (http2d_req_step) req_spdy_step;
	n->base.methods.header_base   = (http2d_req_header_base) req_spdy_header_base;
	n->base.methods.header_add    = (http2d_req_header_add) req_spdy_header_add;
	n->base.methods.header_finish = (http2d_req_header_finish) req_spdy_header_finish;

	/* Return */
	*req_spdy = n;
	return ret_ok;
}


static ret_t
req_spdy_mrproper (http2d_request_spdy_t *req_spdy)
{
	http2d_buffer_mrproper (&req_spdy->header);
	req_spdy->header_num = 0;
}


static void
req_spdy_free (http2d_request_spdy_t *req_spdy)
{
	/* Clean up base object */
	http2d_request_mrproper (REQ(req_spdy));

	/* Mr Proper*/
	req_spdy_mrproper (req_spdy);
	free (req_spdy);
}


static ret_t
req_spdy_header_add (http2d_request_spdy_t *req_spdy,
		     http2d_buffer_t       *key,
		     http2d_buffer_t       *val)
{
	int32_t n;

	/* Render the header entry */
	n = htonl(key->len);
	http2d_buffer_add (&req_spdy->header, (char*)&n, sizeof(int32_t));
	http2d_buffer_add (&req_spdy->header, key->buf, key->len);

	n = htonl(val->len);
	http2d_buffer_add (&req_spdy->header, (char*)&n, sizeof(int32_t));
	http2d_buffer_add (&req_spdy->header, val->buf, val->len);

	/* Update the counter */
	req_spdy->header_num += 1;

	return ret_ok;
}



static ret_t
req_spdy_header_base (http2d_request_spdy_t *req_spdy)
{
	ret_t                ret;
	http2d_buffer_t      key;
	http2d_buffer_t      val;
	cuint_t              str_len;
	const char          *str      = NULL;
	http2d_request_t    *req      = REQ(req_spdy);
	http2d_connection_t *conn     = CONN(req->conn);

	/* Status
	 */
	str_len = 0;
	str     = NULL;

	ret = http2d_http_code_to_string (req->error_code, &str, &str_len);
	if (unlikely ((ret != ret_ok) || (str == NULL) || (str_len == 0))) {
		return ret_error;
	}

	http2d_buffer_fake_str (&key, ":status");
	http2d_buffer_fake (&val, str, str_len);

	req_spdy_header_add (req_spdy, &key, &val);

	/* version
	 */
	str_len = 0;
	str     = NULL;

	ret = http2d_http_version_to_string (conn->protocol.application, &str, &str_len);
	if (unlikely ((ret != ret_ok) || (str == NULL) || (str_len == 0))) {
		return ret_error;
	}

	http2d_buffer_fake_str (&key, ":version");
	http2d_buffer_fake (&val, str, str_len);

	printf ("str '%s'\n", str);

	req_spdy_header_add (req_spdy, &key, &val);

	return ret_ok;
}


static ret_t
req_spdy_header_finish (http2d_request_spdy_t *req_spdy)
{
	int32_t                   header_num;
	int                       buf_len_orig;
	uint8_t                   ctrl[12]      = {0,0,0,0,0,0,0,0,0,0,0,0};
	http2d_request_t         *req           = REQ(req_spdy);
	http2d_connection_t      *conn          = CONN(req->conn);
	http2d_connection_spdy_t *conn_spdy     = CONN_SPDY(req->conn);
	http2d_buffer_t           tmp           = HTTP2D_BUF_INIT;

	/* Add the leading header number */
	header_num = htonl (req_spdy->header_num);
	http2d_buffer_prepend (&req_spdy->header, (char*)&header_num, sizeof(int32_t));

	/* Deflate the header block */
	http2d_spdy_zlib_deflate (&conn_spdy->zst_deflate, &req_spdy->header, &tmp);

	/* Build Control Frame header */
	http2d_spdy_write_syn_reply (ctrl, conn_spdy->ID, 0, tmp.len);

	/* Add Control + Headers */
	http2d_buffer_add        (&conn->buffer_write, ctrl, sizeof(ctrl));
	http2d_buffer_add_buffer (&conn->buffer_write, &tmp);

	http2d_buffer_mrproper (&tmp);
	http2d_buffer_mrproper (&req_spdy->header);

	return ret_ok;
}


ret_t
TEMP_hello_world (http2d_request_t *req)
{
	ret_t                     ret;
	uint8_t                   ctrl[8]   = {0,0,0,0,0,0,0,0};
        http2d_connection_t      *conn      = CONN(req->conn);
	http2d_connection_spdy_t *conn_spdy = CONN_SPDY(req->conn);
	char                      payload[] = "Hello World!!!!!!!!!\n";

	printf ("1 TEMP_hello_world: %d, conn_spdy->ID %d\n", conn->buffer_write.len, conn_spdy->ID);

	http2d_spdy_write_data (ctrl, conn_spdy->ID,
				SPDY_CTRL_FLAG_FIN, sizeof(payload)-1);

        http2d_buffer_add (&conn->buffer_write, ctrl, sizeof(ctrl));
	http2d_buffer_add (&conn->buffer_write, payload, sizeof(payload)-1);

	printf ("2 TEMP_hello_world: %d\n", conn->buffer_write.len);

	return ret_ok;
}


static ret_t
req_spdy_step (http2d_request_spdy_t *req_spdy,
	       int                   *wanted_io)
{
	ret_t             ret;
	http2d_request_t *req  = REQ(req_spdy);

	printf ("request step. phase=%d\n", req->phase);

	switch (req->phase) {
        case phase_spdy_nothing:
		req->phase = phase_spdy_process_header;

        case phase_spdy_process_header:
		req->phase = phase_spdy_setup_request;

        case phase_spdy_setup_request:
		req->phase = phase_spdy_init;

        case phase_spdy_init:
		req->phase = phase_spdy_reading_post;

        case phase_spdy_reading_post:
		req->phase = phase_spdy_add_headers;

        case phase_spdy_add_headers:
		// Testing
		req->error_code = http_ok;

		req_spdy_header_base (req_spdy);
		http2d_request_header_add_common (req);
		http2d_request_header_finish (req);

		TEMP_hello_world (req);

		req->phase = phase_spdy_stepping;

		// TMP
		return ret_ok;

        case phase_spdy_stepping:
		return ret_eof;
		break;

	default:
		SHOULDNT_HAPPEN;
		return ret_error;
	}

	return ret_ok;
}
