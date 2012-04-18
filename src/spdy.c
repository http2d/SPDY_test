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
#include "spdy.h"

#define SPDY_VERSION 3


uint16_t
_read_uint16 (const uint8_t *data)
{
	uint16_t n;
	memcpy(&n, data, sizeof(uint16_t));
	return ntohs(n);
}

uint32_t
_read_uint32 (const uint8_t *data)
{
	uint32_t n;
	memcpy(&n, data, sizeof(uint32_t));
	return ntohl(n);
}

void
_write_uint16 (uint8_t *buf, uint16_t n)
{
	uint16_t x = htons(n);
	memcpy (buf, &x, sizeof(uint16_t));
}

void
_write_uint32 (uint8_t *buf, uint32_t n)
{
	uint32_t x = htonl(n);
	memcpy (buf, &x, sizeof(uint32_t));
}


ret_t
http2d_spdy_read_ctrl (http2d_buffer_t *buf, int pos, http2d_spdy_ctrl_t *ctrl)
{
	char *p = &buf->buf[pos];

	ctrl->version = _read_uint16(p) & SPDY_MASK_VERSION;
	ctrl->type    = _read_uint16(p+2);
	ctrl->flags   = p[4];
	ctrl->length  = _read_uint32(p+4) & SPDY_MASK_LENGTH;

	return ret_ok;
}


void
http2d_spdy_write_ctrl (uint8_t buf[8], uint16_t frame_type, uint8_t flags, uint32_t length)
{
	/* '        '        '         '        '
	 * +------------------------------------+
	 * |1|  version(15)  |  frame_type(16)  |
	 * +------------------------------------+
	 * |Flags(8)|        Length(24)         |
	 * +------------------------------------+
	 */
	_write_uint16 (&buf[0], SPDY_VERSION);
	buf[0] |= (1 << 7);
	_write_uint16 (&buf[2], frame_type);

	_write_uint32 (&buf[4], length);
	buf[4] = flags;

	printf ("WRITE ctrl length: %d\n", length);
	printf ("WRITE ctrl flags: %d\n",  flags);
	printf ("WRITE ctrl type: %d\n",  frame_type);
}


void
http2d_spdy_write_syn_reply (uint8_t buf[12], uint32_t id, uint8_t flags, uint32_t headers_length)
{
	/* '        '        '         '        '
	 * +------------------------------------+  <+
	 * |1|  version(15)  |       2(16)      |   |
	 * +------------------------------------+   |  Control Frame
	 * |Flags(8)|        Length(24)         |   |
	 * +------------------------------------+  <+
	 * |X|           Stream-ID (31bits)     |      Extra 4 bytes
	 * +------------------------------------+
	 */
	http2d_spdy_write_ctrl (buf, SPDY_SYN_REPLY, flags, headers_length + 4);

	_write_uint32 (&buf[8], id);
	buf[8] &= ~(1 << 7);
}


void
http2d_spdy_write_data (uint8_t buf[8], uint32_t id, uint8_t flags, uint32_t length)
{
	/* '        '        '         '        '
	 * +------------------------------------+
	 * |0|          Stream-ID(31)           |
	 * +------------------------------------+
	 * |Flags(8)|        Length(24)         |
	 * +------------------------------------+
	 */

	_write_uint32 (&buf[0], id);
	buf[0] &= ~(1 << 7);

	_write_uint32 (&buf[4], length);
	buf[4] = flags;

	printf ("WRITE data id: %d\n", id);
	printf ("WRITE data length: %d\n", length);
	printf ("WRITE data flags: %d\n",  flags);
}
