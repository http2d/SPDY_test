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
#include "version.h"

ret_t
http2d_version_add (http2d_buffer_t *buf, http2d_server_token_t level)
{
	ret_t ret;

	switch (level) {
	case http2d_version_product:
		ret = http2d_buffer_add_str (buf, "Http2d web server");
		break;
	case http2d_version_minor:
		ret = http2d_buffer_add_str (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION);
		break;
	case http2d_version_minimal:
		ret = http2d_buffer_add_str (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION);
		break;
	case http2d_version_os:
		ret = http2d_buffer_add_str (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE ")");
		break;
	case http2d_version_full:
		ret = http2d_buffer_add_str (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION PACKAGE_PATCH_VERSION " (" OS_TYPE ")");
		break;
	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
	}

	return ret;
}


ret_t
http2d_version_add_w_port (http2d_buffer_t *buf, http2d_server_token_t level, cuint_t port)
{
	ret_t ret;

	switch (level) {
	case http2d_version_product:
		ret = http2d_buffer_add_va (buf, "Http2d web server, Port %d", port);
		break;
	case http2d_version_minor:
		ret = http2d_buffer_add_va (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION ", Port %d", port);
		break;
	case http2d_version_minimal:
		ret = http2d_buffer_add_va (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION ", Port %d", port);
		break;
	case http2d_version_os:
		ret = http2d_buffer_add_va (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE "), Port %d", port);
		break;
	case http2d_version_full:
		ret = http2d_buffer_add_va (buf, "Http2d web server " PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION PACKAGE_PATCH_VERSION " (" OS_TYPE "), Port %d", port);
		break;
	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
	}

	return ret;
}


ret_t
http2d_version_add_simple (http2d_buffer_t *buf, http2d_server_token_t level)
{
	ret_t ret;

	switch (level) {
	case http2d_version_product:
		ret = http2d_buffer_add_str (buf, "Http2d");
		break;
	case http2d_version_minor:
		ret = http2d_buffer_add_str (buf, "Http2d/" PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION);
		break;
	case http2d_version_minimal:
		ret = http2d_buffer_add_str (buf, "Http2d/" PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION);
		break;
	case http2d_version_os:
		ret = http2d_buffer_add_str (buf, "Http2d/" PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION " (" OS_TYPE ")");
		break;
	case http2d_version_full:
		ret = http2d_buffer_add_str (buf, "Http2d/" PACKAGE_MAJOR_VERSION "." PACKAGE_MINOR_VERSION "." PACKAGE_MICRO_VERSION PACKAGE_PATCH_VERSION " (" OS_TYPE ")");
		break;
	default:
		SHOULDNT_HAPPEN;
		ret = ret_error;
	}

	return ret_ok;
}
