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
#include "ctest.h"
#include "buffer.h"

/* Init */
CTEST(buffer, init) {
	   http2d_buffer_t buf;

	   ASSERT_RET_OK (http2d_buffer_init (&buf));
	   ASSERT_EQUAL  (0, buf.len);
	   ASSERT_EQUAL  (0, buf.size);
	   ASSERT_NULL   (buf.buf);
}

/* New */
CTEST(buffer, new) {
	   http2d_buffer_t *buf = NULL;

	   http2d_buffer_new (&buf);
	   ASSERT_NOT_NULL (buf);
}

/* Mr Proper */
CTEST(buffer, mrproper) {
	   http2d_buffer_t buf;

	   http2d_buffer_init (&buf);
	   http2d_buffer_add_str (&buf, "test");
	   ASSERT_NOT_NULL (buf.buf);
	   ASSERT_EQUAL (4, buf.len);

	   http2d_buffer_mrproper (&buf);
	   ASSERT_NULL  (buf.buf);
	   ASSERT_EQUAL (0, buf.len);
}

