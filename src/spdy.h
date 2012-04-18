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

#ifndef HTTP2D_SPDY_H
#define HTTP2D_SPDY_H

#include "common.h"
#include "buffer.h"


/* SPDY enum types
 */
typedef enum {
	SPDY_SYN_STREAM = 1,
	SPDY_SYN_REPLY  = 2,
	SPDY_RST_STREAM = 3,
	SPDY_SETTINGS   = 4,
	SPDY_NOOP       = 5,
	SPDY_PING       = 6,
	SPDY_GOAWAY     = 7,
	SPDY_HEADERS    = 8,
	SPDY_DATA       = 100,
} http2d_spdy_frame_type_t;

typedef enum {
	SPDY_SETTINGS_UPLOAD_BANDWIDTH       = 1,
	SPDY_SETTINGS_DOWNLOAD_BANDWIDTH     = 2,
	SPDY_SETTINGS_ROUND_TRIP_TIME        = 3,
	SPDY_SETTINGS_MAX_CONCURRENT_STREAMS = 4,
	SPDY_SETTINGS_CURRENT_CWND           = 5,
	SPDY_SETTINGS_DOWNLOAD_RETRANS_RATE  = 6,
	SPDY_SETTINGS_INITIAL_WINDOW_SIZE    = 7
} http2d_spdy_settings_id_t;

typedef enum {
	SPDY_OK = 0,
	SPDY_PROTOCOL_ERROR      = 1,
	SPDY_INVALID_STREAM      = 2,
	SPDY_REFUSED_STREAM      = 3,
	SPDY_UNSUPPORTED_VERSION = 4,
	SPDY_CANCEL              = 5,
	SPDY_INTERNAL_ERROR      = 6,
	SPDY_FLOW_CONTROL_ERROR  = 7
} http2d_spdy_status_code_t;

typedef enum {
	SPDY_CTRL_FLAG_NONE           = 0,
	SPDY_CTRL_FLAG_FIN            = 0x1,
	SPDY_CTRL_FLAG_UNIDIRECTIONAL = 0x2
} http2d_spdy_ctrl_flag;


/* Structures
 */
typedef struct {
	uint16_t version;
	uint16_t type;
	uint8_t  flags;
	int32_t  length;
} http2d_spdy_ctrl_t;

/* SYN_STREAM:

+------------------------------------+
|1|    version    |         1        |
+------------------------------------+
|  Flags (8)  |  Length (24 bits)    |
+------------------------------------+
|X|           Stream-ID (31bits)     |
+------------------------------------+
|X| Associated-To-Stream-ID (31bits) |
+------------------------------------+
| Pri|Unused | Slot |                |        3 bits | 5 bits | 8 bits
+-------------------+                |
| Number of Name/Value pairs (int32) |   <+
+------------------------------------+    |
|     Length of name (int32)         |    | This section is the "Name/Value
+------------------------------------+    | Header Block", and is compressed.
|           Name (string)            |    |
+------------------------------------+    |
|     Length of value  (int32)       |    |
+------------------------------------+    |
|          Value   (string)          |    |
+------------------------------------+    |
|           (repeats)                |   <+
*/

typedef struct {
	int32_t              stream_id;
	int32_t              assoc_stream_id;
	uint8_t              pri;
	uint8_t              slot;
//	char               **nv;
} http2d_spdy_syn_stream_t;

typedef struct {
	int32_t              stream_id;
//	char               **nv;
} spdylay_syn_reply;

typedef struct {
	int32_t              stream_id;
//	char               **nv;
} http2d_spdy_headers_t;

typedef struct {
	int32_t            stream_id;
	uint32_t           status_code;
} http2d_spdy_rst_stream_t;

typedef struct {
	int32_t            last_good_id;
	uint32_t           status_code;
} http2d_spdy_go_away_t;


/* Bit masks
 */
#define SPDY_MASK_STREAM_ID 0x7fffffff
#define SPDY_MASK_LENGTH    0xffffff
#define SPDY_MASK_VERSION   0x7fff


/* Constants
 */
#define SPDY_DATA_PAYLOAD_LENGTH 4096          /* The length of DATA frame payload. */
#define SPDY_FRAME_HEAD_LENGTH   8             /* The number of bytes of frame header. */




uint16_t _read_uint16    (const uint8_t *data);
uint32_t _read_uint32    (const uint8_t *data);
void     _write_uint16   (uint8_t *buf, uint16_t n);
void     _write_uint32be (uint8_t *buf, uint32_t n);

ret_t http2d_spdy_read_ctrl       (http2d_buffer_t *buf, int pos, http2d_spdy_ctrl_t *ctrl);
void  http2d_spdy_write_ctrl      (uint8_t buf[8], uint16_t frame_type, uint8_t flags, uint32_t length);
void  http2d_spdy_write_data      (uint8_t buf[8], uint32_t id, uint8_t flags, uint32_t length);
void  http2d_spdy_write_syn_reply (uint8_t buf[12], uint32_t id, uint8_t flags, uint32_t length);


#endif /* HTTP2D_SPDY_H */
