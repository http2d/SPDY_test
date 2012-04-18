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
#include "trace.h"
#include "buffer.h"
#include "util.h"
#include "access.h"
#include "socket.h"
#include "connection.h"

#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif

typedef struct {
	http2d_buffer_t  modules;
	bool             use_syslog;
	bool             print_time;
	bool             print_thread;
	http2d_access_t *from_filter;
} http2d_trace_t;


static http2d_trace_t trace = {
	HTTP2D_BUF_INIT,
	false,
	false,
	false,
	NULL
};


/* http2d_trace_do_trace() calls complex functions that might do
 * some tracing as well, and that would generate a loop. This flag is
 * used to disable the tracing while a previous trace call is ongoin.
 */
static bool disabled = false;


ret_t
http2d_trace_init (void)
{
	const char        *env;
	http2d_buffer_t  tmp = HTTP2D_BUF_INIT;

	/* Read the environment variable
	 */
	env = getenv (TRACE_ENV);
	if (env == NULL)
		return ret_ok;

	/* Set it up
	 */
	http2d_buffer_fake (&tmp, env, strlen(env));
	http2d_trace_set_modules (&tmp);

	return ret_ok;
}


ret_t
http2d_trace_set_modules (http2d_buffer_t *modules)
{
	ret_t            ret;
	char            *p;
	char            *end;
	http2d_buffer_t  tmp = HTTP2D_BUF_INIT;

	/* Store a copy of the modules
	 */
	http2d_buffer_clean (&trace.modules);

	if (trace.from_filter != NULL) {
		http2d_access_free (trace.from_filter);
	}
	trace.from_filter = NULL;

	if (http2d_buffer_case_cmp_str (modules, "none") != 0) {
		http2d_buffer_add_buffer (&trace.modules, modules);
	}

	/* Check the special properties
	 */
	trace.use_syslog   = (strstr (modules->buf, "syslog") != NULL);
	trace.print_time   = (strstr (modules->buf, "time") != NULL);
	trace.print_thread = (strstr (modules->buf, "thread") != NULL);

	/* Even a more special 'from' property
	 */
	p = strstr (modules->buf, "from=");
	if (p != NULL) {
		p += 5;

		end = strchr(p, ',');
		if (end == NULL) {
			end = p + strlen(p);
		}

		if (end > p) {
			ret = http2d_access_new (&trace.from_filter);
			if (ret != ret_ok) return ret_error;

			http2d_buffer_add (&tmp, p, end-p);

			ret = http2d_access_add (trace.from_filter, tmp.buf);
			if (ret != ret_ok) {
				ret = ret_error;
				goto out;
			}
		}
	}

	ret = ret_ok;

out:
	http2d_buffer_mrproper (&tmp);
	return ret;
}


void
http2d_trace_do_trace (const char *entry, const char *file, int line, const char *func, const char *fmt, ...)
{
	ret_t                  ret;
	char                  *p;
	char                  *lentry;
	char                  *lentry_end;
	va_list                args;
	http2d_connection_t *conn;
	http2d_buffer_t     *trace_modules = &trace.modules;
	bool                 do_log        = false;
	http2d_buffer_t      entries       = HTTP2D_BUF_INIT;

	/* Prevents loops
	 */
	if (disabled) {
		return;
	}

	disabled = true;

	/* Return ASAP if nothing is being traced
	 */
	if (http2d_buffer_is_empty (&trace.modules)) {
		goto out;
	}

	/* Check the connection source, if possible
	 */
	if (trace.from_filter != NULL) {
		/* Current conn is available in a thread property
		 */
		conn = CONN (pthread_getspecific (thread_connection_ptr));

		/* No conn, no trace entry
		 */
		if (conn == NULL) {
			goto out;
		}

		if (conn->socket.socket < 0) {
			goto out;
		}

		/* Skip the trace if the conn doesn't match
		 */
		ret = http2d_access_ip_match (trace.from_filter, &conn->socket);
		if (ret != ret_ok) {
			goto out;
		}
	}

	/* Also, check for 'all'
	 */
	p = strstr (trace_modules->buf, "all");
	if (p == NULL) {
		/* Parse the In-code module string
		 */
		http2d_buffer_add (&entries, entry, strlen(entry));

		for (lentry = entries.buf;;) {
			lentry_end = strchr (lentry, ',');
			if (lentry_end) *lentry_end = '\0';

			/* Check the type
			 */
			p = strstr (trace_modules->buf, lentry);
			if (p) {
				char *tmp = p + strlen(lentry);
				if ((*tmp == '\0') || (*tmp == ',') || (*tmp == ' '))
					do_log = true;
			}

			if ((lentry_end == NULL) || (do_log))
				break;

			lentry = lentry_end + 1;
		}

		/* Return if trace entry didn't match with the configured list
		 */
		if (! do_log) {
			goto out;
		}
	}

	/* Format the message and log it:
	 * 'entries' is not needed at this stage, reuse it
	 */
	http2d_buffer_clean (&entries);
	if (trace.print_thread) {
		int        len;
		char       tmp[32+1];
		static int longest_len = 0;

		len = snprintf (tmp, 32+1, "%llX", (unsigned long long) HTTP2D_THREAD_SELF);
		longest_len = MAX (longest_len, len);

		http2d_buffer_add_str    (&entries, "{0x");
		http2d_buffer_add_char_n (&entries, '0', longest_len - len);
		http2d_buffer_add        (&entries, tmp, len);
		http2d_buffer_add_str    (&entries, "} ");
	}

	if (trace.print_time) {
		http2d_buffer_add_char (&entries, '[');
		http2d_buf_add_bogonow (&entries, true);
		http2d_buffer_add_str  (&entries, "] ");
	}

	http2d_buffer_add_va (&entries, "%18s:%04d (%30s): ", file, line, func);

	va_start (args, fmt);
	http2d_buffer_add_va_list (&entries, (char *)fmt, args);
	va_end (args);

	if (trace.use_syslog) {
		http2d_syslog (LOG_DEBUG, &entries);
	} else {
#ifdef HAVE_FLOCKFILE
		flockfile (stdout);
#endif
		fprintf (stdout, "%s", entries.buf);
#ifdef HAVE_FUNLOCKFILE
		funlockfile (stdout);
#endif
	}

out:
	http2d_buffer_mrproper (&entries);
	disabled = false;
}


ret_t
http2d_trace_get_trace (http2d_buffer_t **buf_ref)
{
	if (buf_ref == NULL)
		return ret_not_found;

	*buf_ref = &trace.modules;
	return ret_ok;
}


int
http2d_trace_is_tracing (void)
{
	return (! http2d_buffer_is_empty (&trace.modules));
}
