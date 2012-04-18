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

#ifndef HTTP2D_BUFFER_H
#define HTTP2D_BUFFER_H

#include "common.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

typedef struct {
	char    *buf;        /**< Memory chunk           */
	cuint_t  size;       /**< Total amount of memory */
	cuint_t  len;        /**< Length of the string   */
} http2d_buffer_t;

#define BUF(x) ((http2d_buffer_t *)(x))
#define HTTP2D_BUF_INIT         {NULL, 0, 0}
#define HTTP2D_BUF_INIT_FAKE(s) {s, sizeof(s), sizeof(s)-1}
#define HTTP2D_BUF_SLIDE_NONE   INT_MIN

#define http2d_buffer_is_empty(b)        (BUF(b)->len == 0)
#define http2d_buffer_add_str(b,s)       http2d_buffer_add (b, s, sizeof(s)-1)
#define http2d_buffer_prepend_str(b,s)   http2d_buffer_prepend (b, s, sizeof(s)-1)
#define http2d_buffer_prepend_buf(b,s)   http2d_buffer_prepend (b, (s)->buf, (s)->len)
#define http2d_buffer_cmp_str(b,s)       http2d_buffer_cmp      (b, (char *)(s), sizeof(s)-1)
#define http2d_buffer_case_cmp_str(b,s)  http2d_buffer_case_cmp (b, (char *)(s), sizeof(s)-1)
#define http2d_buffer_fake_str(b,s)      http2d_buffer_fake (b, s, sizeof(s)-1)

ret_t http2d_buffer_new                  (http2d_buffer_t **buf);
ret_t http2d_buffer_free                 (http2d_buffer_t  *buf);
ret_t http2d_buffer_init                 (http2d_buffer_t  *buf);
ret_t http2d_buffer_mrproper             (http2d_buffer_t  *buf);
void  http2d_buffer_fake                 (http2d_buffer_t  *buf, const char *str, cuint_t len);

void  http2d_buffer_clean                (http2d_buffer_t  *buf);
ret_t http2d_buffer_dup                  (http2d_buffer_t  *buf, http2d_buffer_t **dup);
void  http2d_buffer_swap_buffers         (http2d_buffer_t  *buf, http2d_buffer_t *second);

ret_t http2d_buffer_add                  (http2d_buffer_t  *buf, const char *txt, size_t size);
ret_t http2d_buffer_add_long10           (http2d_buffer_t  *buf, clong_t lNum);
ret_t http2d_buffer_add_llong10          (http2d_buffer_t  *buf, cllong_t lNum);
ret_t http2d_buffer_add_ulong10          (http2d_buffer_t  *buf, culong_t ulNum);
ret_t http2d_buffer_add_ullong10         (http2d_buffer_t  *buf, cullong_t ulNum);
ret_t http2d_buffer_add_ulong16          (http2d_buffer_t  *buf, culong_t ulNum);
ret_t http2d_buffer_add_ullong16         (http2d_buffer_t  *buf, cullong_t ulNum);
ret_t http2d_buffer_add_uint16be         (http2d_buffer_t  *buf, uint16_t n);
ret_t http2d_buffer_add_uint32be         (http2d_buffer_t  *buf, uint32_t n);
ret_t http2d_buffer_add_va               (http2d_buffer_t  *buf, const char *format, ...);
ret_t http2d_buffer_add_va_fixed         (http2d_buffer_t  *buf, const char *format, ...);
ret_t http2d_buffer_add_va_list          (http2d_buffer_t  *buf, const char *format, va_list args);
ret_t http2d_buffer_add_char             (http2d_buffer_t  *buf, char c);
ret_t http2d_buffer_add_char_n           (http2d_buffer_t  *buf, char c, int n);
ret_t http2d_buffer_add_buffer           (http2d_buffer_t  *buf, http2d_buffer_t *buf2);
ret_t http2d_buffer_add_buffer_slice     (http2d_buffer_t  *buf, http2d_buffer_t *buf2, ssize_t begin, ssize_t end);
ret_t http2d_buffer_add_fsize            (http2d_buffer_t  *buf, CST_OFFSET size);

ret_t http2d_buffer_prepend              (http2d_buffer_t  *buf, const char *txt, size_t size);

cint_t http2d_buffer_cmp                 (http2d_buffer_t  *buf, char *txt, cuint_t txt_len);
cint_t http2d_buffer_cmp_buf             (http2d_buffer_t  *buf, http2d_buffer_t *buf2);
cint_t http2d_buffer_case_cmp            (http2d_buffer_t  *buf, char *txt, cuint_t txt_len);
cint_t http2d_buffer_case_cmp_buf        (http2d_buffer_t  *buf, http2d_buffer_t *buf2);

ret_t http2d_buffer_read_file            (http2d_buffer_t  *buf, char *filename);
ret_t http2d_buffer_read_from_fd         (http2d_buffer_t  *buf, int fd, size_t size, size_t *ret_size);

ret_t http2d_buffer_move_to_begin        (http2d_buffer_t  *buf, cuint_t pos);
ret_t http2d_buffer_drop_ending          (http2d_buffer_t  *buf, cuint_t num_chars);
ret_t http2d_buffer_multiply             (http2d_buffer_t  *buf, int num);
ret_t http2d_buffer_swap_chars           (http2d_buffer_t  *buf, char a, char b);
ret_t http2d_buffer_remove_dups          (http2d_buffer_t  *buf, char c);
ret_t http2d_buffer_remove_string        (http2d_buffer_t  *buf, char *string, int string_len);
ret_t http2d_buffer_remove_chunk         (http2d_buffer_t  *buf, cuint_t from, cuint_t len);
ret_t http2d_buffer_replace_string       (http2d_buffer_t  *buf, const char *subs, int subs_len, const char *repl, int repl_len);
ret_t http2d_buffer_substitute_string    (http2d_buffer_t  *bufsrc, http2d_buffer_t *bufdst, char *subs, int subs_len, char *repl, int repl_len);
ret_t http2d_buffer_trim                 (http2d_buffer_t  *buf);
ret_t http2d_buffer_insert               (http2d_buffer_t  *buf, char *txt, size_t txt_len, size_t pos);
ret_t http2d_buffer_insert_buffer        (http2d_buffer_t  *buf, http2d_buffer_t *src, size_t pos);

ret_t http2d_buffer_get_utf8_len         (http2d_buffer_t  *buf, cuint_t *len);
ret_t http2d_buffer_ensure_addlen        (http2d_buffer_t  *buf, size_t alen);
ret_t http2d_buffer_ensure_size          (http2d_buffer_t  *buf, size_t size);

int    http2d_buffer_is_ending           (http2d_buffer_t  *buf, char c);
char   http2d_buffer_end_char            (http2d_buffer_t  *buf);
size_t http2d_buffer_cnt_spn             (http2d_buffer_t  *buf, cuint_t offset, const char *str);
size_t http2d_buffer_cnt_cspn            (http2d_buffer_t  *buf, cuint_t offset, const char *str);

crc_t http2d_buffer_crc32                (http2d_buffer_t  *buf);
ret_t http2d_buffer_encode_base64        (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_decode_base64        (http2d_buffer_t  *buf);
ret_t http2d_buffer_encode_md5           (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_encode_md5_digest    (http2d_buffer_t  *buf);
ret_t http2d_buffer_encode_sha1          (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_encode_sha1_digest   (http2d_buffer_t  *buf);
ret_t http2d_buffer_encode_sha1_base64   (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_encode_sha512        (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_encode_sha512_digest (http2d_buffer_t  *buf);
ret_t http2d_buffer_encode_sha512_base64 (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_encode_hex           (http2d_buffer_t  *buf, http2d_buffer_t *encoded);
ret_t http2d_buffer_decode_hex           (http2d_buffer_t  *buf);
ret_t http2d_buffer_unescape_uri         (http2d_buffer_t  *buf);
ret_t http2d_buffer_escape_uri           (http2d_buffer_t  *buf, http2d_buffer_t *src);
ret_t http2d_buffer_escape_uri_delims    (http2d_buffer_t  *buf, http2d_buffer_t *src);
ret_t http2d_buffer_escape_arg           (http2d_buffer_t  *buf, http2d_buffer_t *src);
ret_t http2d_buffer_add_escape_html      (http2d_buffer_t  *buf, http2d_buffer_t *src);
ret_t http2d_buffer_escape_html          (http2d_buffer_t  *buf, http2d_buffer_t *src);
ret_t http2d_buffer_add_comma_marks      (http2d_buffer_t  *buf);

ret_t http2d_buffer_to_lowcase           (http2d_buffer_t  *buf);
ret_t http2d_buffer_split_lines          (http2d_buffer_t  *buf, int columns, const char *indent);

ret_t http2d_buffer_print_debug          (http2d_buffer_t  *buf, int length);

#endif /* HTTP2D_BUFFER_H */
