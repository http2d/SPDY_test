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

#ifndef HTTP2D_ERROR_LOG_H
#define HTTP2D_ERROR_LOG_H

#include "common.h"
#include "errors_defs.h"

typedef enum {
	http2d_err_warning,
	http2d_err_error,
	http2d_err_critical
} http2d_error_type_t;

typedef struct {
	int         id;
	const char *title;
	const char *description;
	const char *admin_url;
	const char *debug;
	int         show_backtrace;
} http2d_error_t;

#define HTTP2D_ERROR(x) ((http2d_error_t *)(x))


#if defined(__GNUC__) || ( defined(__SUNPRO_C) && __SUNPRO_C > 0x590 )
# define LOG_WARNING(e_num,arg...)     http2d_error_log (http2d_err_warning,  __FILE__, __LINE__, e_num, ##arg)
# define LOG_WARNING_S(e_num)          http2d_error_log (http2d_err_warning,  __FILE__, __LINE__, e_num)
# define LOG_ERROR(e_num,arg...)       http2d_error_log (http2d_err_error,    __FILE__, __LINE__, e_num, ##arg)
# define LOG_ERROR_S(e_num)            http2d_error_log (http2d_err_error,    __FILE__, __LINE__, e_num)
# define LOG_CRITICAL(e_num,arg...)    http2d_error_log (http2d_err_critical, __FILE__, __LINE__, e_num, ##arg)
# define LOG_CRITICAL_S(e_num)         http2d_error_log (http2d_err_critical, __FILE__, __LINE__, e_num)
# define LOG_ERRNO(syserror,type,e_num,arg...) http2d_error_errno_log (syserror, type, __FILE__, __LINE__, e_num, ##arg)
# define LOG_ERRNO_S(syserror,type,e_num)      http2d_error_errno_log (syserror, type, __FILE__, __LINE__, e_num)
#else
# define LOG_WARNING(e_num,arg...)     http2d_error_log (http2d_err_warning,  __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_WARNING_S(e_num)          http2d_error_log (http2d_err_warning,  __FILE__, __LINE__, e_num)
# define LOG_ERROR(e_num,arg...)       http2d_error_log (http2d_err_error,    __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_ERROR_S(e_num)            http2d_error_log (http2d_err_error,    __FILE__, __LINE__, e_num)
# define LOG_CRITICAL(e_num,arg...)    http2d_error_log (http2d_err_critical, __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_CRITICAL_S(e_num)         http2d_error_log (http2d_err_critical, __FILE__, __LINE__, e_num)
# define LOG_ERRNO(syserror,type,e_num,arg...) http2d_error_errno_log (syserror, type, __FILE__, __LINE__, e_num, __VA_ARGS__)
# define LOG_ERRNO_S(syserror,type,e_num)      http2d_error_errno_log (syserror, type, __FILE__, __LINE__, e_num)
#endif

ret_t http2d_error_log         (http2d_error_type_t  type,
				const char          *filename,
				int                  line,
				int                  error_num, ...);

ret_t http2d_error_errno_log   (int                  errnumber,
				http2d_error_type_t  type,
				const char          *filename,
				int                  line,
				int                  error_num, ...);

/* ret_t http2d_error_set_default (http2d_logger_writer_t *writer); */

#endif /* HTTP2D_ERROR_LOG_H */
