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

#ifndef HTTP2D_MACROS_H
#define HTTP2D_MACROS_H

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "consts.h"

#define __http2d_func__ __func__

/* Booleans
 */
#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

/* NULL
 */
#ifndef NULL
#  ifdef __cplusplus
#    define NULL        (0L)
#  else /* !__cplusplus */
#    define NULL        ((void*) 0)
#  endif /* !__cplusplus */
#endif

/* Compiler version */
#if defined(__GNUC__)
# define __GNUC_VERSION	\
 	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
# else
# define __GNUC_VERSION	0
#endif

/* Compiler options */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
# define likely(x)   __builtin_expect((x),1)
# define unlikely(x) __builtin_expect((x),0)
# define not_ok(x)   __builtin_expect(x != ret_ok, 0)
# define lt_ok(x)    __builtin_expect(x <  ret_ok, 0)
#else
# define likely(x)   (x)
# define unlikely(x) (x)
# define not_ok(x)   (x != ret_ok)
# define lt_ok(x)    (x <  ret_ok)
#endif

#if __GNUC_VERSION >= 30000
# define must_check  __attribute__ ((warn_unused_result))
# define no_return   __attribute__ ((noreturn))
#else
# define must_check
# define no_return
#endif

/* Useful strings
 */
#define LF_LF     "\n\n"        /* EOHs (End Of Headers) */
#define CRLF_CRLF "\r\n\r\n"    /* EOHs (End Of Headers) */
#define CRLF      "\r\n"        /* EOH (End Of Header Line) */
#define LWS       " \t\r\n"     /* HTTP linear white space */
#define LBS       " \t"         /* HTTP linear blank space */
#define CHR_CR    '\r'          /* 0x0D: Carriage return */
#define CHR_LF    '\n'          /* 0x0A: Line feed (new line) */
#define CHR_SP    ' '           /* Space */
#define CHR_HT    '\t'          /* Horizontal tab */

#define HTTP2D_CHAR_IS_WHITE(_ch)     (((_ch) == ' ') || ((_ch) == '\t') || ((_ch) == '\n'))
#define HTTP2D_CHAR_IS_UPPERCASE(_ch) (((_ch) >= 'A') && ((_ch) <= 'Z'))
#define HTTP2D_CHAR_IS_LOWERCASE(_ch) (((_ch) >= 'a') && ((_ch) <= 'z'))
#define HTTP2D_CHAR_IS_DIGIT(_ch)     (((_ch) >= '0') && ((_ch) <= '9'))
#define HTTP2D_CHAR_TO_LOWER(_ch)     ((_ch) | 32)
#define HTTP2D_CHAT_TO_UPPER(_ch)     ((_ch) & ~32)

/* Conversions
 */
#define POINTER_TO_INT(pointer) ((long)(pointer))
#define INT_TO_POINTER(integer) ((void*)((long)(integer)))

/* Bit masks
 */
#define BIT_SET(var,bit)    var |= (bit)
#define BIT_UNSET(var,bit)  var &= ~(bit)

/* TIME
 */
#define HTTP2D_BOGONOW   ev_now(EV_DEFAULT_UC)

/* Format string for off_t and size_t
 */
#if (SIZEOF_OFF_T == SIZEOF_UNSIGNED_LONG_LONG)
# define FMT_OFFSET "%llu"
# define FMT_OFFSET_HEX "%llx"
# define CST_OFFSET unsigned long long
#elif (SIZEOF_OFF_T == SIZEOF_UNSIGNED_LONG)
# define FMT_OFFSET "%lu"
# define FMT_OFFSET_HEX "%lx"
# define CST_OFFSET unsigned long
#else
# error Unknown size of off_t
#endif

#if (SIZEOF_SIZE_T == SIZEOF_UNSIGNED_INT)
# define FMT_SIZE "%d"
# define FMT_SIZE_HEX "%x"
# define CST_SIZE unsigned int
#elif (SIZEOF_SIZE_T == SIZEOF_UNSIGNED_LONG_LONG)
# define FMT_SIZE "%llu"
# define FMT_SIZE_HEX "%llx"
# define CST_SIZE unsigned long long
#else
# error Unknown size of size_t
#endif

/* UWORD
 */
#if (SIZEOF_UNSIGNED_LONG == 4)
# define UWORD32 unsigned long
#elif (SIZEOF_UNSIGNED_INT == 4)
# define UWORD32 unsigned int
#else
# error "I do not know what to use for a UWORD32"
#endif

/* VA copy
 */
#ifndef va_copy
# ifdef __va_copy
#  define va_copy(DEST,SRC) __va_copy((DEST),(SRC))
# else
#  ifdef HAVE_VA_LIST_AS_ARRAY
#   define va_copy(DEST,SRC) (*(DEST) = *(SRC))
#  else
#   define va_copy(DEST,SRC) ((DEST) = (SRC))
#  endif
# endif
#endif

/* Temporal vars pair
 */
#define HTTP2D_TEMP_VARS(obj, size)             \
        const unsigned int obj ## _size = size; \
        char obj[size]

/* Macros
 */
#define HTTP2D_MK_TYPE(type)         http2d_ ## type ## _t
#define HTTP2D_MK_NEW(obj,type)      http2d_ ## type ## _new (&obj)
#define HTTP2D_DCL_POINTER(obj,type) http2d_ ## type ## _t *obj

#define RETURN_IF_FAIL(expr,ret)					\
	do {								\
		if (!(expr)) {						\
			PRINT_ERROR ("assertion `%s' failed\n", #expr);	\
			return (ret);					\
		}							\
	} while(0)

#define SHOULDNT_HAPPEN							    \
	do {							   	    \
		fprintf (stderr,					    \
			 "file %s:%d (%s): this shouldn't have happened\n", \
			 __FILE__, __LINE__, __http2d_func__);	            \
		fflush (stderr);					    \
	} while (0)

#define RET_TO_STR(r)  (r == ret_no_sys)?         "ret_no_sys"         : \
		       (r == ret_nomem)?          "ret_nomem"          : \
		       (r == ret_deny)?           "ret_deny"           : \
		       (r == ret_error)?          "ret_error"          : \
		       (r == ret_ok)?             "ret_ok"             : \
		       (r == ret_eof)?            "ret_eof"            : \
		       (r == ret_eof_have_data)?  "ret_eof_have_data"  : \
		       (r == ret_not_found)?      "ret_not_found"      : \
		       (r == ret_file_not_found)? "ret_file_not_found" : \
		       (r == ret_eagain)?         "ret_eagain"         : \
		       (r == ret_ok_and_sent)?    "ret_ok_and_sent"    : \
		       "ret_???"


#define RET_UNKNOWN(ret)		       			     \
	do {					      		     \
		fprintf (stderr,			   	     \
			 "file %s:%d (%s): unknown ret code: %s\n",  \
			 __FILE__, __LINE__,                         \
		        __http2d_func__, RET_TO_STR(ret));	     \
		fflush (stderr);				     \
	} while (0)

#define UNUSED(x) ((void)(x))

/* Declare and allocate a new struct.
 */
#define HTTP2D_NEW_STRUCT(obj,type)				 \
	HTTP2D_DCL_POINTER(obj,type) = (HTTP2D_MK_TYPE(type) *)	 \
		malloc (sizeof(HTTP2D_MK_TYPE(type)));		 \
	RETURN_IF_FAIL (obj != NULL, ret_nomem)

/* Declare and initialize a new object.
 */
#define HTTP2D_NEW(obj,type)                   \
	HTTP2D_DCL_POINTER(obj,type) = NULL;   \
	HTTP2D_MK_NEW(obj,type);               \
	RETURN_IF_FAIL (obj != NULL, ret_nomem)

/* Automatic functions:
 * These macros implement _new/_free by using _init/_mrproper.
 */
#define HTTP2D_ADD_FUNC_NEW(klass)  \
	ret_t                                                         \
	http2d_ ## klass ## _new (http2d_ ## klass ## _t **obj) {     \
		ret_t ret;                                            \
		HTTP2D_NEW_STRUCT (n, klass);			      \
								      \
		ret = http2d_ ## klass ## _init (n);		      \
		if (unlikely (ret != ret_ok)) {			      \
			free (n);				      \
			return ret;				      \
		}						      \
		                                                      \
		*obj = n;                                             \
		return ret_ok;                                        \
	}

#define HTTP2D_ADD_FUNC_FREE(klass)  \
	ret_t                                                         \
	http2d_ ## klass ## _free (http2d_ ## klass ## _t *obj) {     \
		if ((obj) == NULL)				      \
			return ret_ok;				      \
								      \
		http2d_ ## klass ## _mrproper (obj);		      \
		                                                      \
		free (obj);                                           \
		return ret_ok;                                        \
	}


/* Printing macros
 */
#if defined(__GNUC__) || ( defined(__SUNPRO_C) && __SUNPRO_C > 0x590 )
# define PRINT_MSG(fmt,arg...)    do { fprintf(stderr, fmt, ##arg); fflush(stderr); } while(0)
# define PRINT_ERROR(fmt,arg...)  do { fprintf(stderr, "%s:%d - "fmt, __FILE__, __LINE__, ##arg); fflush(stderr); } while(0)
#else
# define PRINT_MSG(fmt,...)       do { fprintf(stderr, fmt, __VA_ARGS__); fflush(stderr); } while(0)
# define PRINT_ERROR(fmt,...)     do { fprintf(stderr, "%s:%d - "fmt, __FILE__, __LINE__, __VA_ARGS__); fflush(stderr); } while(0)
#endif

#ifdef DEBUG
# ifdef __GNUC__
#  define PRINT_DEBUG(fmt,arg...) do { fprintf(stdout, "%s:%d - " fmt,__FILE__,__LINE__,##arg); fflush(stdout); } while (0)
# else
#  define PRINT_DEBUG(fmt,...)    do { fprintf(stdout, "%s:%d - " fmt,__FILE__,__LINE__,__VA_ARGS__); fflush(stdout); } while (0)
# endif
#else
# ifdef __GNUC__
#  define PRINT_DEBUG(fmt,arg...) do { } while(0)
# else
#  define PRINT_DEBUG(fmt,...)    do { } while(0)
# endif
#endif

#define PRINT_ERROR_S(str)        PRINT_ERROR("%s",str)
#define PRINT_MSG_S(str)          PRINT_MSG("%s",str)

/* Tracing facility
 */
#define TRACE_ENV "TRACE"

#ifdef TRACE_ENABLED
# ifdef __GNUC__
#  define TRACE(fmt,arg...) http2d_trace_do_trace (fmt, __FILE__, __LINE__, __http2d_func__, ##arg)
# else
#  define TRACE(fmt,...) http2d_trace_do_trace (fmt, __FILE__, __LINE__, __http2d_func__, __VA_ARGS__)
# endif
#else
# ifdef __GNUC__
#  define TRACE(section,fmt,arg...) do { } while(0)
# else
#  define TRACE(section,fmt,...) do { } while(0)
# endif
#endif

#ifndef MIN
# define MIN(x,y) ((x<y) ? x : y)
#endif

#ifndef MAX
# define MAX(x,y) ((x>y) ? x : y)
#endif

#endif /* HTTP2D_MACROS_H */
