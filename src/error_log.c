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
#include "error_log.h"
#include "nullable.h"
#include "util.h"

/* Include the error information */
#include "errors.h"

static void
skip_args (va_list ap, const char *prev_string)
{
	const char *p = prev_string;

	p = strchr (prev_string, '%');
	while (p) {
		p++;
		switch (*p) {
		case 's':
			va_arg (ap, char *);
			break;
		case 'd':
			va_arg (ap, int);
			break;
		default:
			break;
		}

		p = strchr (p, '%');
	}
}


static void
render (http2d_error_type_t   type,
	const char           *filename,
	int                   line,
	int                   error_num,
	va_list               ap,
	http2d_buffer_t      *output)
{
	va_list               ap_tmp;
	const http2d_error_t *error;

	UNUSED (error_num);

	/* Time */
	http2d_buffer_add_char (output, '[');
	http2d_buf_add_bogonow (output, false);
	http2d_buffer_add_str  (output, "] ");

	/* Error type */
	switch (type) {
	case http2d_err_warning:
		http2d_buffer_add_str (output, "(warning)");
		break;
	case http2d_err_error:
		http2d_buffer_add_str (output, "(error)");
		break;
	case http2d_err_critical:
		http2d_buffer_add_str (output, "(critical)");
		break;
	default:
		SHOULDNT_HAPPEN;
	}

	/* Source */
	http2d_buffer_add_va (output, " %s:%d - ", filename, line);

	/* Get the error */
	error = &__http2d_errors[error_num];
	if (unlikely (error->id != error_num)) {
		return;
	}

	/* Error */
	va_copy (ap_tmp, ap);
	http2d_buffer_add_va_list (output, error->title, ap_tmp);
	skip_args (ap, error->title);

	/* Description */
	if (error->description) {
		va_copy (ap_tmp, ap);
		http2d_buffer_add_str     (output, " | ");
		http2d_buffer_add_va_list (output, error->description, ap_tmp);
		http2d_buffer_add_char    (output, '\n');
		skip_args (ap, error->title);
	}

	http2d_buffer_add_char (output, '\n');
}


static ret_t
report_error (http2d_buffer_t *buf)
{
	fprintf (stderr, "%s\n", buf->buf);
	fflush (stderr);
	return ret_ok;
}


ret_t
http2d_error_log (http2d_error_type_t  type,
		  const char          *filename,
		  int                  line,
		  int                  error_num, ...)
{
	va_list            ap;
	http2d_buffer_t  error_str = HTTP2D_BUF_INIT;

/* render (http2d_error_type_t   type, */
/* 	const char           *filename, */
/* 	int                   line, */
/* 	int                   error_num, */
/* 	const http2d_error_t *error, */
/* 	http2d_buffer_t      *output, */
/* 	va_list               ap) */


	/* Render the error message
	 */
	va_start (ap, error_num);
	render (type, filename, line, error_num, ap, &error_str);
	va_end (ap);

	/* Report it
	 */
	report_error (&error_str);

	/* Clean up
	 */
	http2d_buffer_mrproper (&error_str);
	return ret_ok;
}

ret_t
http2d_error_errno_log (int                  errnumber,
			http2d_error_type_t  type,
			const char          *filename,
			int                  line,
			int                  error_num, ...)
{
	// TODO
	return ret_ok;
}
