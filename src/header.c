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
#include "header.h"


ret_t
http2d_header_init (http2d_header_t *hdr)
{
	http2d_buffer_init (&hdr->method);
	http2d_buffer_init (&hdr->path);
	http2d_buffer_init (&hdr->version);
	http2d_buffer_init (&hdr->host);
	http2d_buffer_init (&hdr->scheme);

	return ret_ok;
}


ret_t
http2d_header_mrproper (http2d_header_t *hdr)
{
	http2d_buffer_mrproper (&hdr->method);
	http2d_buffer_mrproper (&hdr->path);
	http2d_buffer_mrproper (&hdr->version);
	http2d_buffer_mrproper (&hdr->host);
	http2d_buffer_mrproper (&hdr->scheme);

	return ret_ok;
}


ret_t
http2d_header_clean (http2d_header_t *hdr)
{
	http2d_buffer_clean (&hdr->method);
	http2d_buffer_clean (&hdr->path);
	http2d_buffer_clean (&hdr->version);
	http2d_buffer_clean (&hdr->host);
	http2d_buffer_clean (&hdr->scheme);

	return ret_ok;
}


ret_t
http2d_header_add_header (http2d_header_t *hdr,
			  const char *key, int key_len,
			  const char *val, int val_len)
{
#define SPDY_CMP(str) ((key_len-1 == sizeof(str)-1) && (strncasecmp (key+1, str, key_len-1) == 0))

	if (key[0] == ':') {
		if (SPDY_CMP("path")) {
			http2d_buffer_clean (&hdr->path);
			http2d_buffer_add   (&hdr->path, val, val_len);
		} else if (SPDY_CMP("method")) {
			http2d_buffer_clean (&hdr->method);
			http2d_buffer_add   (&hdr->method, val, val_len);
		} else if (SPDY_CMP("scheme")) {
			http2d_buffer_clean (&hdr->scheme);
			http2d_buffer_add   (&hdr->scheme, val, val_len);
		} else if (SPDY_CMP("version")) {
			http2d_buffer_clean (&hdr->version);
			http2d_buffer_add   (&hdr->version, val, val_len);
		} else if (SPDY_CMP("host")) {
			http2d_buffer_clean (&hdr->host);
			http2d_buffer_add   (&hdr->host, val, val_len);
		} else {
			SHOULDNT_HAPPEN;
		}

		return ret_ok;
	}

	printf ("path '%s'\n", hdr->path.buf);
	printf ("method '%s'\n", hdr->method.buf);
	printf ("scheme '%s'\n", hdr->scheme.buf);
	printf ("version '%s'\n", hdr->version.buf);
	printf ("host '%s'\n", hdr->host.buf);

	// TODO: Handle regular header

	return ret_ok;
}
