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
#include "threading.h"

#ifdef HAVE_PTHREAD
# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE) || defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
pthread_mutexattr_t http2d_mutexattr_fast;
pthread_mutexattr_t http2d_mutexattr_errorcheck;
# endif
#endif

/* Thread Local Storage variables */
#ifdef HAVE_PTHREAD
pthread_key_t thread_error_writer_ptr = 0;
pthread_key_t thread_connection_ptr   = 0;
#endif


ret_t
http2d_threading_init (void)
{
#ifdef HAVE_PTHREAD
# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE)
	pthread_mutexattr_init (&http2d_mutexattr_fast);
	pthread_mutexattr_settype (&http2d_mutexattr_fast,
	                           PTHREAD_MUTEX_NORMAL);
	pthread_mutexattr_init (&http2d_mutexattr_errorcheck);
	pthread_mutexattr_settype (&http2d_mutexattr_errorcheck,
	                           PTHREAD_MUTEX_ERRORCHECK);

# elif defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
	pthread_mutexattr_init (&http2d_mutexattr_fast);
	pthread_mutexattr_setkind_np (&http2d_mutexattr_fast,
	                              PTHREAD_MUTEX_ADAPTIVE_NP);
	pthread_mutexattr_init (&http2d_mutexattr_errorcheck);
	pthread_mutexattr_setkind_np (&http2d_mutexattr_errorcheck,
	                              PTHREAD_MUTEX_ERRORCHECK_NP);
# endif
#endif

//	pthread_key_create (&thread_error_writer_ptr, NULL);
	pthread_key_create (&thread_connection_ptr,   NULL);
//	pthread_key_create (&thread_request_ptr,      NULL);

	return ret_ok;
}


ret_t
http2d_threading_free (void)
{
#ifdef HAVE_PTHREAD
# if defined(HAVE_PTHREAD_MUTEXATTR_SETTYPE) || defined(HAVE_PTHREAD_MUTEXATTR_SETKIND_NP)
	pthread_mutexattr_destroy (&http2d_mutexattr_fast);
	pthread_mutexattr_destroy (&http2d_mutexattr_errorcheck);
# endif
#endif

//	pthread_key_delete (thread_error_writer_ptr);
	pthread_key_delete (thread_connection_ptr);
//	pthread_key_delete (thread_request_ptr);

	return ret_ok;
}

