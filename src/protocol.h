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

#ifndef HTTP2D_PROTOCOL_H
#define HTTP2D_PROTOCOL_H

/*                           or
 *  .------------------------------------.
 *  | Application  | HTTP 1.1 | HTTP 2.0 |
 *  |--------------+----------+----------|
 *  | Session      |   SPDY   |   None   |
 *  |--------------+----------+----------|
 *  | Presentation |   SSL    |   None   |
 *  |--------------+----------+----------|
 *  | Transport    |         TCP         |
 *  .------------------------------------.
 */


/* Application */
typedef enum {
	prot_app_unknown,
	prot_app_http10,
	prot_app_http11,
	prot_app_http20,
} http2d_prot_application_t;

/* Session */
typedef enum {
	prot_session_none,
	prot_session_SPDY,
} http2d_prot_session_t;

/* Presentation */
typedef enum {
	prot_presentation_none,
	prot_presentation_SSL,
} http2d_prot_presentation_t;


/* Protocols
 */
typedef struct {
	http2d_prot_application_t  application;
	http2d_prot_session_t      session;
	http2d_prot_presentation_t presentation;
} http2d_protocol_t;


ret_t http2d_protocol_init (http2d_protocol_t *protocol);
ret_t http2d_protocol_copy (http2d_protocol_t *protocol, http2d_protocol_t *target);


#endif /* HTTP2D_PROTOCOL_H */
