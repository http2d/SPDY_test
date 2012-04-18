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

#ifndef HTTP2D_COMMON_INTERNAL_H
#define HTTP2D_COMMON_INTERNAL_H

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include "macros.h"
#include "threading.h"
#include "constants.h"

#ifndef _WIN32
# if defined HAVE_ENDIAN_H
#  include <endian.h>
# elif defined HAVE_MACHINE_ENDIAN_H
#  include <machine/endian.h>
# elif defined HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
# elif defined HAVE_SYS_MACHINE_H
#  include <sys/machine.h>
# elif defined HAVE_SYS_ISA_DEFS_H
#  include <sys/isa_defs.h>
# else
#  error "Can not include endian.h"
# endif
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_VARARGS_H
# include <sys/varargs.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#endif

#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#elif HAVE_STDINT_H
# include <stdint.h>
#else
# error "Can not include inttypes or stdint"
#endif

#ifdef HAVE_PTHREAD
# include <pthread.h>
#endif

#ifdef HAVE_SCHED_H
# include <sched.h>
#endif

/* IMPORTANT:
 * Cross compilers should define BYTE_ORDER in CFLAGS
 */
#ifndef BYTE_ORDER
# ifndef  LITTLE_ENDIAN
#  define LITTLE_ENDIAN  1234    /* LSB first: i386, vax */
# endif
# ifndef BIG_ENDIAN
#  define BIG_ENDIAN     4321    /* MSB first: 68000, ibm, net */
# endif
# ifndef PDP_ENDIAN
#  define PDP_ENDIAN     3412    /* LSB first in word, MSW first in long */
# endif
# ifdef WORDS_BIGENDIAN
#   define BYTE_ORDER  BIG_ENDIAN
# else
#   define BYTE_ORDER  LITTLE_ENDIAN
# endif
#endif

/* Int limit
 */
#ifndef INT_MAX
# if (SIZEOF_INT == 4)
#  define INT_MAX 0x7fffffffL          /* 2^32 - 1 */
# elif (SIZEOF_INT == 8)
#  define INT_MAX 0x7fffffffffffffffL  /* 2^64 - 1 */
# else
#  error "Can't define INT_MAX"
# endif
#endif

/* Long limit
 */
#ifndef LONG_MAX
# if (SIZEOF_LONG == 4)
#  define LONG_MAX 0x7fffffffL
# elif (SIZEOF_LONG == 8)
#  define LONG_MAX 0x7fffffffffffffffL
# else
#  error "Can't define LONG_MAX"
# endif
#endif

/* time_t limit
 */
#ifndef TIME_MAX
# if (SIZEOF_TIME_T == SIZEOF_INT)
#  define TIME_MAX ((time_t)INT_MAX)
# elif (SIZEOF_TIME_T == SIZEOF_LONG)
#  define TIME_MAX ((time_t)LONG_MAX)
# else
#  error "Can't define TIME_MAX"
# endif
#endif

/* Missing constants
 */
#ifndef O_BINARY
# define O_BINARY 0
#endif

/* Pthread
 */
#define HTTP2D_MUTEX_T(n)           pthread_mutex_t n
#define HTTP2D_RWLOCK_T(n)          pthread_rwlock_t n
#define HTTP2D_THREAD_JOIN(t)       pthread_join(t,NULL)
#define HTTP2D_THREAD_KILL(t,s)     pthread_kill(t,s)
#define HTTP2D_THREAD_SELF          pthread_self()

#define HTTP2D_THREAD_PROP_GET(p)   pthread_getspecific(p)
#define HTTP2D_THREAD_PROP_SET(p,v) pthread_setspecific(p,v)
#define HTTP2D_THREAD_PROP_NEW(p,f) pthread_key_create2(p,f)

#define HTTP2D_MUTEX_LOCK(m)        pthread_mutex_lock(m)
#define HTTP2D_MUTEX_UNLOCK(m)      pthread_mutex_unlock(m)
#define HTTP2D_MUTEX_INIT(m,n)      pthread_mutex_init(m,n)
#define HTTP2D_MUTEX_DESTROY(m)     pthread_mutex_destroy(m)
#define HTTP2D_MUTEX_TRY_LOCK(m)    pthread_mutex_trylock(m)

#define HTTP2D_RWLOCK_INIT(m,n)     do {				          \
		                         memset (m, 0, sizeof(pthread_rwlock_t)); \
					 pthread_rwlock_init(m,n);                \
                                       } while(0)
#define HTTP2D_RWLOCK_READER(m)     pthread_rwlock_rdlock(m)
#define HTTP2D_RWLOCK_WRITER(m)     pthread_rwlock_wrlock(m)
#define HTTP2D_RWLOCK_TRYREADER(m)  pthread_rwlock_tryrdlock(m)
#define HTTP2D_RWLOCK_TRYWRITER(m)  pthread_rwlock_trywrlock(m)
#define HTTP2D_RWLOCK_UNLOCK(m)     pthread_rwlock_unlock(m)
#define HTTP2D_RWLOCK_DESTROY(m)    pthread_rwlock_destroy(m)

#ifdef HAVE_SCHED_YIELD
# define HTTP2D_THREAD_YIELD         sched_yield()
#else
# define HTTP2D_THREAD_YIELD
#endif

#endif /* HTTP2D_COMMON_INTERNAL_H */
