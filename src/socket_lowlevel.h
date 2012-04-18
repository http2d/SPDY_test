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

#ifndef HTTP2D_SOCKET_LOWLEVEL_H
#define HTTP2D_SOCKET_LOWLEVEL_H

#include "common-internal.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <sys/types.h>
#include <netdb.h>

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
# include <sys/un.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#ifdef _WIN32
# define SOCK_ERRNO() WSAGetLastError()
#else
# define SOCK_ERRNO() errno
#endif

#ifdef INET6_ADDRSTRLEN
# define CHE_INET_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#  ifdef INET_ADDRSTRLEN
#    define CHE_INET_ADDRSTRLEN INET_ADDRSTRLEN
#  else
#    define CHE_INET_ADDRSTRLEN 16
#  endif
#endif

#ifndef AF_LOCAL
# define AF_LOCAL AF_UNIX
#endif

#ifndef SUN_LEN
#define SUN_LEN(sa)						\
	(strlen((sa)->sun_path) +				\
	 (size_t)(((struct sockaddr_un*)0)->sun_path))
#endif

#ifndef SUN_ABSTRACT_LEN
#define SUN_ABSTRACT_LEN(sa)					\
	(strlen((sa)->sun_path+1) + 2 +				\
	 (size_t)(((struct sockaddr_un*)0)->sun_path))
#endif


typedef struct {
	union {
		struct in_addr  addr_ipv4;
		struct in6_addr addr_ipv6;
	}              addr;
	unsigned short family;
} http2d_in_addr_t;


#endif /* HTTP2D_SOCKET_LOWLEVEL_H */
