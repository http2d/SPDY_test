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
#include "connection-spdy.h"
#include "socket-ssl.h"
#include "request.h"
#include "thread.h"
#include "avl.h"
#include "spdy.h"
#include "spdy-zlib.h"


static void  conn_spdy_free (http2d_connection_spdy_t *conn);
static ret_t conn_spdy_step (http2d_connection_spdy_t *conn);


ret_t
http2d_connection_spdy_new (http2d_connection_spdy_t **conn)
{
	ret_t ret;
	HTTP2D_NEW_STRUCT (n, connection_spdy);

	/* Base class */
	ret = http2d_connection_init_base (&n->base);
	if (unlikely (ret != ret_ok))
		return ret;

	/* Zlib */
	ret = http2d_spdy_zlib_inflate_init (&n->zst_inflate);
	if (unlikely (ret != ret_ok))
		return ret_error;

	ret = http2d_spdy_zlib_deflate_init (&n->zst_deflate);
	if (unlikely (ret != ret_ok))
		return ret_error;

	/* SPDY properties */
	n->ID         = 0;
	n->going_away = false;

	/* Methods */
	n->base.methods.free = (http2d_conn_free) conn_spdy_free;
	n->base.methods.step = (http2d_conn_step) conn_spdy_step;

	*conn = n;
	return ret_ok;
}


static void
conn_spdy_free (http2d_connection_spdy_t *conn)
{
	/* Clean up base object */
	http2d_connection_mrproper (&conn->base);

	/* Zlib */
	deflateEnd (&conn->zst_inflate);
	deflateEnd (&conn->zst_deflate);

	/* Free the obj */
	free (conn);
}


static ret_t
read_name_value (z_stream *zst, http2d_buffer_t *buf, int pos, http2d_header_t *header)
{
	ret_t            ret;
	int              header_num;
	char            *p           = &buf->buf[pos];
	http2d_buffer_t  tmp         = HTTP2D_BUF_INIT;

	ret = http2d_spdy_zlib_inflate (zst, buf, pos, &tmp);
	if (ret != ret_ok) {
		return ret;
	}

	p = tmp.buf;
	header_num = _read_uint32 (p);
	p += 4;

	for (int n=0; n<header_num; n++) {
		int         len_key;
		int         len_val;
		const char *val;

		/* Read the lengths */
		len_key = _read_uint32 (p);
		len_val = _read_uint32 (p + 4 + len_key);
		val     = p + 4 + len_key + 4;

		/* Store the header entry */
		ret = http2d_header_add_header (header, p+4, len_key, val, len_val);
		if (ret != ret_ok) {
			return ret_error;
		}
	}

	return ret_ok;
}


static ret_t
handle_SPDY_SYN_STREAM (http2d_connection_spdy_t *conn,
			http2d_buffer_t          *buf,
			http2d_spdy_ctrl_t       *ctrl,
			int                      *pos)
{
	ret_t                     ret;
	http2d_avl_t              headers;
	http2d_spdy_syn_stream_t  syn_ctrl;
	char                     *p;
	http2d_request_t         *req       = NULL;

	/* GO_AWAY received? Ignore it. */
	if (conn->going_away)
		return ret_ok;

	/* Create a new request */
	ret = _http2d_connection_new_req (CONN(conn), &req);
	if (unlikely (ret != ret_ok)) {
		goto error;
	}

	/* Copy the Control frame */
	p = buf->buf + *pos;

	/* Process SYN Stream */
	syn_ctrl.stream_id       = _read_uint32(p)   & SPDY_MASK_STREAM_ID;
	syn_ctrl.assoc_stream_id = _read_uint32(p+4) & SPDY_MASK_STREAM_ID;
	syn_ctrl.pri             = (p[8] >> 6) & 0x3;
	syn_ctrl.slot            = p[9];
	p += 10;

	conn->ID = syn_ctrl.stream_id;

	printf ("stream id = %d\n",       syn_ctrl.stream_id);
	printf ("stream assoc id = %d\n", syn_ctrl.assoc_stream_id);
	printf ("priority  = %d\n",       syn_ctrl.pri);
	printf ("slot      = %d\n",       syn_ctrl.slot);

	/* Parse headers */
	ret = read_name_value (&conn->zst_inflate, buf, p - buf->buf, &req->header);
	if (ret != ret_ok) {
		goto error;
	}

	/* Skip the SYN frame */
	*pos += ctrl->length;
	return ret_ok;

error:
	if (req != NULL)
		http2d_request_free (req);

	return ret_error;
}


static ret_t
handle_SPDY_GOAWAY (http2d_connection_spdy_t *conn,
		    http2d_buffer_t          *buf,
		    http2d_spdy_ctrl_t       *ctrl,
		    int                      *pos)
{
	ret_t                  ret;
	http2d_spdy_go_away_t  away_ctrl;
	char                  *p;

	/* Copy the Control frame */
	p = buf->buf + *pos;

	/* Process GOAWAY Stream */
	away_ctrl.last_good_id = _read_uint32(p) & SPDY_MASK_STREAM_ID;
	away_ctrl.status_code  = _read_uint32(p+4);

	*pos += ctrl->length;

	printf ("go away: last good=%d, status code: %d\n", away_ctrl.last_good_id, away_ctrl.status_code);

	/* Update the connection phase */
	CONN_BASE(conn)->phase = phase_conn_spdy_shutdown;
	conn->going_away  = true;

	return ret_ok;
}

static ret_t
handle_SPDY_RST_STREAM (http2d_connection_spdy_t *conn,
			http2d_buffer_t          *buf,
			http2d_spdy_ctrl_t       *ctrl,
			int                      *pos)
{
	ret_t                     ret;
	http2d_spdy_rst_stream_t  rst_ctrl;
	char                     *p;

	/* Copy the Control frame */
	p = buf->buf + *pos;

	/* Process GOAWAY Stream */
	rst_ctrl.stream_id   = _read_uint32(p) & SPDY_MASK_STREAM_ID;
	rst_ctrl.status_code = _read_uint32(p+4);

	printf ("RESET!!!!!!!!!!!!!!!!!!! reason=%d\n", rst_ctrl.status_code);

	switch (rst_ctrl.status_code) {
	case SPDY_OK:
		break;
	case SPDY_PROTOCOL_ERROR:
		break;
	case SPDY_INVALID_STREAM:
		break;
	case SPDY_REFUSED_STREAM:
		break;
	case SPDY_UNSUPPORTED_VERSION:
		break;
	case SPDY_CANCEL:
		break;
	case SPDY_INTERNAL_ERROR:
		break;
	case SPDY_FLOW_CONTROL_ERROR:
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	*pos += ctrl->length;
	return ret_ok;
}


static ret_t
spdy_process_frame (http2d_connection_spdy_t *conn,
		    http2d_buffer_t          *buf,
		    int                      *pos)
{
	ret_t               ret;
	http2d_spdy_ctrl_t  ctrl;
	char               *p     = NULL;

	/* We need at least a header
	 */
	printf ("buf->len %d\n", buf->len);
	if (buf->len < SPDY_FRAME_HEAD_LENGTH)
		return ret_eagain;

	http2d_spdy_read_ctrl (buf, *pos, &ctrl);

	printf ("version = %d\n", ctrl.version);
	printf ("type    = %d\n", ctrl.type);
	printf ("flags   = %d\n", ctrl.flags);
	printf ("length  = %d (buf=%d)\n", ctrl.length, buf->len);

	if (ctrl.version != 3) {
		printf ("================version!=3(len %d)===============\n", buf->len);
		http2d_buffer_print_debug (buf, buf->len);
		printf ("=================================================\n");
		return ret_error;
	}

	/* Is the frame complete?
	 */
	if (ctrl.length > buf->len - (*pos + 8))
		return ret_ok;

	*pos += 8;

	/* Parse frame
	 */
	switch (ctrl.type) {
	case SPDY_SYN_STREAM:
		ret = handle_SPDY_SYN_STREAM (conn, buf, &ctrl, pos);
		if (ret != ret_ok) {
			return ret;
		}
		break;

	case SPDY_RST_STREAM:
		ret = handle_SPDY_RST_STREAM (conn, buf, &ctrl, pos);
		if (ret != ret_ok) {
			return ret;
		}
		break;

	case SPDY_GOAWAY:
		ret = handle_SPDY_GOAWAY (conn, buf, &ctrl, pos);
		if (ret != ret_ok) {
			return ret;
		}
		break;

	default:
		printf ("!!!!UNKNOWN FRAME!!!! %d\n", ctrl.type);
		exit(1);
		SHOULDNT_HAPPEN;
		break;
	}

	return ret_ok;
}


static ret_t
spdy_process_buffer (http2d_connection_spdy_t *conn,
		     http2d_buffer_t          *buf)
{
	ret_t ret;
	int   pos  = 0;

	if (buf->len < 12)
		return ret_ok;

	while (buf->len > 8) {
		ret = spdy_process_frame (conn, buf, &pos);
		printf ("buf->len %d, pos %d, ret %d\n", buf->len, pos, ret);
		switch (ret) {
		case ret_ok:
			http2d_buffer_move_to_begin (buf, pos);
			break;
		case ret_eagain:
			return ret_ok;
		case ret_error:
			return ret;
		default:
			SHOULDNT_HAPPEN;
			return ret;
		}
	}

	return ret;
}


static ret_t
do_io (http2d_connection_spdy_t *conn)
{
	ret_t                ret;
	http2d_list_t       *i, *j;
	int                  wanted_io = 0;
	http2d_connection_t *conn_base = CONN_BASE(conn);
	http2d_thread_t     *thd       = THREAD(conn_base->thread);

	printf ("SPDY do io\n");

	/* Read
	 */
	ret = _http2d_connection_do_read (conn_base, &wanted_io);
	switch (ret) {
	case ret_ok:
		break;
	case ret_eagain:
		if (wanted_io != 0) {
			http2d_connection_move_to_polling (conn_base, wanted_io);
		}
		return ret_ok;
	case ret_eof:
		http2d_thread_recycle_conn (thd, conn_base);
		return ret_ok;
	default:
		RET_UNKNOWN (ret);
	}

	/* Process what's in the buffer
	 */
	if (! http2d_buffer_is_empty (&conn_base->buffer_read))
	{
		ret = spdy_process_buffer (conn, &conn_base->buffer_read);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			goto write;
		case ret_error:
			http2d_thread_recycle_conn (thd, conn_base);
			break;
		default:
			SHOULDNT_HAPPEN;
			goto write;
		}
	}

	/* Process requests
	 */
	list_for_each_safe (i, j, &conn_base->requests) {
		ret = http2d_request_step (REQ(i), &wanted_io);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eof:
			printf ("del req %p\n", i);
			http2d_list_del (&REQ(i)->listed);
			http2d_request_free (REQ(i));
			break;
		default:
			RET_UNKNOWN(ret);
		}
	}

write:
	/* Write
	 */
	if ((! http2d_buffer_is_empty (&conn_base->buffer_write)) ||
	    (! http2d_buffer_is_empty (&conn_base->buffer_write_ssl)))
	{
		ret = _http2d_connection_do_write (conn_base, &wanted_io);
		switch (ret) {
		case ret_ok:
			break;
		case ret_eagain:
			if (wanted_io != 0) {
				http2d_connection_move_to_polling (conn_base, wanted_io);
			}
			return ret_ok;
		case ret_eof:
			http2d_thread_recycle_conn (thd, conn_base);
			return ret_ok;
		default:
			RET_UNKNOWN (ret);
		}
	}

	return ret_ok;
}


static ret_t
conn_spdy_step (http2d_connection_spdy_t *conn)
{
	ret_t                ret;
	int                  wanted_io = 0;
	http2d_connection_t *conn_base = CONN_BASE(conn);
	http2d_thread_t     *thd       = THREAD(conn_base->thread);

	switch (conn_base->phase) {
	case phase_conn_spdy_handshake:
		if (conn_base->socket.is_tls == TLS) {
			ret = _http2d_socket_ssl_init (&conn_base->socket, conn_base, &wanted_io);
			switch (ret) {
			case ret_ok:
				break;
			case ret_eof:
				return ret;
			case ret_eagain:
				if (wanted_io != 0) {
					http2d_connection_move_to_polling (conn_base, wanted_io);
				}
				return ret_ok;

			default:
				RET_UNKNOWN(ret);
				return ret_error;
			}
		}
		conn_base->phase = phase_conn_spdy_io;

	case phase_conn_spdy_io:
		ret = do_io (conn);
		switch (ret) {
		case ret_ok:
			return ret_ok;
		case ret_eof:
			goto case_phase_conn_spdy_shutdown;
		case ret_eagain:
			/* Already moved to polling */
			return ret_ok;
		default:
			RET_UNKNOWN (ret);
		}
		break;

	case phase_conn_spdy_shutdown:
	case_phase_conn_spdy_shutdown:
		ret = _http2d_connection_do_shutdown (conn_base);
		if (ret != ret_ok) {
			return ret;
		}
		conn_base->phase = phase_conn_spdy_shutdown;

	case phase_conn_spdy_linger_read:
		wanted_io = -1;

		ret = _http2d_connection_do_linger_read (conn_base, &wanted_io);
		switch (ret) {
		case ret_eagain:
			if (wanted_io != -1) {
				http2d_connection_move_to_polling (conn_base, wanted_io);
				return ret_ok;
			}
			break;
		case ret_eof:
		case ret_error:
			return ret_eof;
		default:
			RET_UNKNOWN(ret);
			return ret_eof;
		}
		break;
	default:
		break;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}
