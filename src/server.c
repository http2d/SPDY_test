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
#include "server.h"
#include "server-cfg.h"
#include "bind.h"
#include "ncpus.h"
#include "thread.h"
#include "list.h"
#include "config_reader.h"
#include "error_log.h"
#include "virtual_server.h"
#include "util.h"

#define ENTRIES "server,core"


ret_t
http2d_server_init (http2d_server_t *srv)
{
	int   re;
	ret_t ret;
	int   ncpus = -1;

	INIT_LIST_HEAD (&srv->binds);
	INIT_LIST_HEAD (&srv->threads);
	INIT_LIST_HEAD (&srv->vservers);

//	srv->main_loop = ev_loop_new (EVFLAG_AUTO);
	srv->main_loop = ev_default_loop (EVFLAG_AUTO);


	/* Launch threads
	 */
	re = dcc_ncpus (&ncpus);
	if (unlikely (re != 0) || (ncpus == -1))
		return ret_error;

	for (int n=0; n < ncpus * 2; n++) {
		http2d_thread_t *thread;

		ret = http2d_thread_new (&thread, srv);
		if (unlikely (ret != ret_ok))
			return ret_error;

		http2d_list_add (&thread->listed, &srv->threads);
	}

	/* Properties
	 */
	http2d_buffer_init (&srv->alternate_protocol);
	http2d_config_node_init (&srv->config);

	return ret_ok;
}


ret_t
http2d_server_mrproper (http2d_server_t *srv)
{
	http2d_list_t *i, *j;

	list_for_each_safe (i, j, &srv->threads) {
		http2d_thread_free (THREAD(i));
	}

	list_for_each_safe (i, j, &srv->binds) {
		http2d_bind_free (BIND(i));
	}

	list_for_each_safe (i, j, &srv->vservers) {
		http2d_virtual_server_free (VSERVER(i));
	}

	http2d_buffer_mrproper (&srv->alternate_protocol);
	http2d_config_node_mrproper (&srv->config);

	ev_loop_destroy (srv->main_loop);
	return ret_ok;
}


ret_t
http2d_server_run (http2d_server_t *srv)
{
	ev_ref (srv->main_loop);
	ev_loop (srv->main_loop, 0);

	return ret_ok;
}


static ret_t
print_banner (http2d_server_t *srv)
{
	ret_t            ret;
	const char      *method;
	http2d_list_t   *i;
	http2d_buffer_t *buf;
	size_t           b       = 0;
	size_t           len     = 0;
	http2d_buffer_t  n       = HTTP2D_BUF_INIT;

	/* First line
	 */
	http2d_buffer_add_va (&n, "HTTP2d %s (%s): ", PACKAGE_VERSION, __DATE__);

	/* Ports
	 */
	http2d_list_get_len (&srv->binds, &len);
	if (len <= 1) {
		http2d_buffer_add_str (&n, "Listening on port ");
	} else {
		http2d_buffer_add_str (&n, "Listening on ports ");
	}

	list_for_each (i, &srv->binds) {
		http2d_bind_t *bind = BIND(i);
		bool           spdy = bind->protocol.session == prot_session_SPDY;
		bool           ssl  = bind->protocol.presentation == prot_presentation_SSL;

		b += 1;
		if (! http2d_buffer_is_empty(&bind->ip)) {
			if (strchr (bind->ip.buf, ':')) {
				http2d_buffer_add_va (&n, "[%s]", bind->ip.buf);
			} else {
				http2d_buffer_add_buffer (&n, &bind->ip);
			}
		} else {
			http2d_buffer_add_str (&n, "ALL");
		}

		http2d_buffer_add_char (&n, ':');
		http2d_buffer_add_ulong10 (&n, bind->port);

		if (ssl && spdy) {
			http2d_buffer_add_str (&n, "(SSL,SPDY)");
		} else if (ssl) {
			http2d_buffer_add_str (&n, "(SSL)");
		} else if (spdy) {
			http2d_buffer_add_str (&n, "(SPDY)");
		}

		if (b < len)
			http2d_buffer_add_str (&n, ", ");
	}

	/* Polling method
	 */
	unsigned int evbackend;
	evbackend = ev_backend (THREAD(srv->threads.next)->loop);

	http2d_buffer_add_str (&n, ", polling with ");
	switch (evbackend) {
	case EVBACKEND_SELECT:
		http2d_buffer_add_str (&n, "select");
		break;
	case EVBACKEND_POLL:
		http2d_buffer_add_str (&n, "poll");
		break;
	case EVBACKEND_EPOLL:
		http2d_buffer_add_str (&n, "epoll");
		break;
	case EVBACKEND_KQUEUE:
		http2d_buffer_add_str (&n, "kqueue");
		break;
	case EVBACKEND_DEVPOLL:
		http2d_buffer_add_str (&n, "devpoll");
		break;
	case EVBACKEND_PORT:
		http2d_buffer_add_str (&n, "port");
		break;
	default:
		http2d_buffer_add_str (&n, "unknown");
		break;
	}

	/* /\* File descriptor limit */
	/*  *\/ */
	/* http2d_buffer_add_va (&n, ", %d fds system limit, max. %d connections", */
	/* 			http2d_fdlimit, srv->conns_max); */

	/* /\* I/O-cache */
	/*  *\/ */
	/* if (srv->iocache) { */
	/* 	http2d_buffer_add_str (&n, ", caching I/O"); */
	/* } */

	/* Threading stuff
	 */
	http2d_list_get_len (&srv->threads, &len);
	http2d_buffer_add_va (&n, ", %d threads", len);

/* 	http2d_buffer_add_va (&n, ", %d connections per thread", srv->main_thread->conns_max); */

	/* Trace
	 */
#ifdef TRACE_ENABLED
	ret = http2d_trace_get_trace (&buf);
	if ((ret == ret_ok) &&
	    (! http2d_buffer_is_empty (buf)))
	{
		http2d_buffer_add_va (&n, ", tracing '%s'", buf->buf);
	}
#endif

	/* Print it to stdout
	 */
	http2d_buffer_split_lines (&n, TERMINAL_WIDTH, NULL);
	fprintf (stdout, "%s\n", n.buf);
	fflush (stdout);

	http2d_buffer_mrproper (&n);

	return ret_ok;
}




ret_t
http2d_server_read_conf (http2d_server_t *srv,
			 http2d_buffer_t *fullpath)
{
	ret_t ret;

        /* Load the configuration file
         */
        ret = http2d_config_reader_parse (&srv->config, fullpath);
        if (ret != ret_ok)
		return ret_error;

	/* Configure the server
	 */
        ret = _http2d_server_config (srv, &srv->config);
        if (ret != ret_ok)
		return ret;

        /* Free the configuration tree
         */
        ret = http2d_config_node_mrproper (&srv->config);
        if (ret != ret_ok)
		return ret_error;

	print_banner (srv);

        return ret_ok;
}


ret_t
http2d_server_initialize (http2d_server_t *srv)
{
	ret_t          ret;
	http2d_list_t *i;

	/* Binds
	 */
	if (http2d_list_empty (&srv->binds)) {
		LOG_CRITICAL (HTTP2D_ERROR_SERVER_NO_BIND);
		return ret_error;
	}

	list_for_each (i, &srv->binds) {
		ret = http2d_bind_init_port (BIND(i), 65534, true, http2d_version_full);
		if (ret != ret_ok) {

			return ret;
		}
	}


	return ret_ok;
}


ret_t
http2d_server_get_vserver (http2d_server_t          *srv,
			   http2d_buffer_t          *name,
			   http2d_connection_t      *conn,
			   http2d_virtual_server_t **vsrv)
{
	// TODO

	/* Safety net
	 */
	*vsrv = VSERVER(srv->vservers.next);
	return ret_ok;
}
