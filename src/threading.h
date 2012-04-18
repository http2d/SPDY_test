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

#ifndef HTTP2D_THREADING_H
#define HTTP2D_THREADING_H

#include "common.h"
#include <pthread.h>

# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE) || defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
extern pthread_mutexattr_t http2d_mutexattr_fast;
extern pthread_mutexattr_t http2d_mutexattr_errorcheck;
# endif

# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE) || defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
#  define HTTP2D_MUTEX_FAST (&http2d_mutexattr_fast)
#  define HTTP2D_MUTEX_ERRORCHECK (&http2d_mutexattr_errorcheck)
# else
#  define HTTP2D_MUTEX_FAST NULL
#  define HTTP2D_MUTEX_ERRORCHECK NULL
# endif

// extern pthread_key_t thread_error_writer_ptr;
extern pthread_key_t thread_connection_ptr;
// extern pthread_key_t thread_request_ptr;

ret_t http2d_threading_init (void);
ret_t http2d_threading_free (void);

#endif /* HTTP2D_BOGOTIME_H */
