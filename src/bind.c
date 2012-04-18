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
#include "bind.h"
#include "util.h"
#include "error_log.h"
#include "server.h"
#include "connection.h"
#include "thread.h"
#include "connection-spdy.h"
#include "connection-http.h"


ret_t
http2d_bind_new (http2d_bind_t **bind, void *srv)
{
	HTTP2D_NEW_STRUCT(n,bind);

	INIT_LIST_HEAD (&n->listed);

	http2d_buffer_init (&n->ip);
	http2d_socket_init (&n->socket);
	n->port = 0;
	n->id   = 0;
	n->srv  = srv;

	http2d_buffer_init (&n->server_string);
	http2d_buffer_init (&n->server_string_w_port);
	http2d_buffer_init (&n->server_address);
	http2d_buffer_init (&n->server_port);

	http2d_protocol_init (&n->protocol);

	n->accept_continuous     = 0;
	n->accept_continuous_max = 0;
	n->accept_recalculate    = 0;

	*bind = n;
	return ret_ok;
}


ret_t
http2d_bind_free (http2d_bind_t *bind)
{
	http2d_socket_close (&bind->socket);
	http2d_socket_mrproper (&bind->socket);

	http2d_buffer_mrproper (&bind->ip);

	http2d_buffer_mrproper (&bind->server_string);
	http2d_buffer_mrproper (&bind->server_string_w_port);
	http2d_buffer_mrproper (&bind->server_address);
	http2d_buffer_mrproper (&bind->server_port);

	free (bind);
	return ret_ok;
}


static ret_t
build_strings (http2d_bind_t         *bind,
	       http2d_server_token_t  token)
{
	ret_t ret;
	char  server_ip[CHE_INET_ADDRSTRLEN+1];

	/* server_string_
	 */
	http2d_buffer_clean (&bind->server_string_w_port);
	ret = http2d_version_add_w_port (&bind->server_string_w_port,
					 token, bind->port);
	if (ret != ret_ok)
		return ret;

	http2d_buffer_clean (&bind->server_string);
	ret = http2d_version_add_simple (&bind->server_string, token);
	if (ret != ret_ok)
		return ret;

	/* server_address
	 */
	http2d_socket_ntop (&bind->socket, server_ip, sizeof(server_ip)-1);
	http2d_buffer_add (&bind->server_address, server_ip, strlen(server_ip));

	/* server_port
	 */
	http2d_buffer_add_va (&bind->server_port, "%d", bind->port);
	return ret_ok;
}


ret_t
http2d_bind_configure (http2d_bind_t        *bind,
		       http2d_config_node_t *conf)
{
	ret_t            ret;
	http2d_buffer_t *buf;
	bool             enabled;

	/* Read configuration parameters
	 */
	ret = http2d_atoi (conf->key.buf, &bind->id);
	if (unlikely (ret != ret_ok))
		return ret_error;

	ret = http2d_config_node_read (conf, "interface", &buf);
	if (ret == ret_ok) {
		http2d_buffer_mrproper (&bind->ip);
		http2d_buffer_add_buffer (&bind->ip, buf);
	}

	ret = http2d_config_node_read_int (conf, "port", &bind->port);
	if (ret != ret_ok) {
		LOG_CRITICAL_S (HTTP2D_ERROR_BIND_PORT_NEEDED);
		return ret_error;
	}

	ret = http2d_config_node_read_bool (conf, "tls", &enabled);
	if ((ret == ret_ok) && (enabled)) {
		bind->socket.is_tls         = TLS;
		bind->protocol.presentation = prot_presentation_SSL;
	}

	ret = http2d_config_node_read_bool (conf, "spdy", &enabled);
	if ((ret == ret_ok) && (enabled)) {
		bind->protocol.session = prot_session_SPDY;
	}

	return ret_ok;
}


ret_t
http2d_bind_set_default (http2d_bind_t *bind)
{
	bind->port = 80;
	return ret_ok;
}


static ret_t
set_socket_opts (int socket)
{
	ret_t                    ret;
#ifdef SO_ACCEPTFILTER
        struct accept_filter_arg afa;
#endif

	/* Set 'close-on-exec'
	 */
	ret = http2d_fd_set_closexec (socket);
	if (ret != ret_ok)
		return ret;

	/* To re-bind without wait to TIME_WAIT. It prevents 2MSL
	 * delay on accept.
	 */
	ret = http2d_fd_set_reuseaddr (socket);
	if (ret != ret_ok)
		return ret;

	/* Set no-delay mode:
	 * If no clients are waiting, accept() will return -1 immediately
	 */
	ret = http2d_fd_set_nodelay (socket, true);
	if (ret != ret_ok)
		return ret;

	/* TCP_DEFER_ACCEPT:
	 * Allows a bind to be awakened only when data arrives on the socket.
	 * Takes an integer value (seconds), this can bound the maximum number of
	 * attempts TCP will make to complete the connection.
	 *
	 * Gives clients 5secs to send the first data packet
	 */
#ifdef TCP_DEFER_ACCEPT
	on = 5;
	setsockopt (socket, SOL_TCP, TCP_DEFER_ACCEPT, &on, sizeof(on));
#endif

	/* SO_ACCEPTFILTER:
	 * FreeBSD accept filter for HTTP:
	 *
	 * http://www.freebsd.org/cgi/man.cgi?query=accf_http
	 */
#ifdef SO_ACCEPTFILTER
	memset (&afa, 0, sizeof(afa));
	strcpy (afa.af_name, "httpready");

	setsockopt (socket, SOL_SOCKET, SO_ACCEPTFILTER, &afa, sizeof(afa));
#endif

	return ret_ok;
}


static void
io_cb (struct ev_loop *loop, ev_io *w, int revents)
{
	ret_t                 ret;
        http2d_sockaddr_t     new_sa;
	int                   new_fd   = -1;
	http2d_bind_t        *bind     = list_entry (w, http2d_bind_t, IO);
	http2d_server_t      *srv      = SRV(bind->srv);
	http2d_connection_t  *conn     = NULL;
	static http2d_list_t *prev_thd = NULL;

	/* Accept fd
	 */
	ret = http2d_socket_accept_fd (&bind->socket, &new_fd, &new_sa);
        if ((ret != ret_ok) || (new_fd == -1)) {
		printf ("error1\n");
		goto error;
        }

	/* Create connection
	 */
	if (bind->protocol.session == prot_session_SPDY) {
		http2d_connection_spdy_t *n;
		ret = http2d_connection_spdy_new (&n);
		conn = CONN(n);
	} else {
		http2d_connection_http_t *n;
		ret = http2d_connection_http_new (&n);
		conn = CONN(n);
	}

	if ((ret != ret_ok) || (conn == NULL)) {
		printf ("error2\n");
		goto error;
	}

	/* Set the connection up
	 */
	ret = http2d_socket_set_sockaddr (&conn->socket, new_fd, &new_sa);
	if ((ret != ret_ok) || (conn == NULL)) {
		printf ("error3\n");
		goto error;
	}

	/* Copy the connection type
	 */
	conn->bind          = bind;
	conn->socket.is_tls = bind->socket.is_tls;

	http2d_protocol_copy (&conn->protocol, &bind->protocol);

	/* TODO: */ conn->protocol.application = prot_app_http11; /* TMP! */

	/* Assign the connection to a thread */
	if (unlikely (prev_thd == NULL)) {
		prev_thd = srv->threads.next;
	} else {
		prev_thd = list_next_circular (&srv->threads, prev_thd);
	}

	ret = http2d_thread_add_conn (THREAD(prev_thd), conn);
	if (unlikely (ret != ret_ok)) {
		printf ("error4\n");
		goto error;
	}

	return;


error:
	http2d_fd_close (new_fd);

	if (conn != NULL) {
		http2d_connection_free (conn);
	}
}


static ret_t
init_socket (http2d_bind_t *bind, int family)
{
	ret_t ret;
	int   fd;

	/* Create the socket, and set its properties
	 */
	ret = http2d_socket_create_fd (&bind->socket, family);
	if ((ret != ret_ok) || (SOCKET_FD(&bind->socket) < 0)) {
		return ret_error;
	}

	fd = SOCKET_FD(&bind->socket);

	/* Set the socket properties
	 */
	ret = set_socket_opts (fd);
	if (ret != ret_ok) {
		goto error;
	}

	/* Bind the socket
	 */
	ret = http2d_socket_bind (&bind->socket, bind->port, &bind->ip);
	if (ret != ret_ok) {
		goto error;
	}

	/* Add it to EV
	 */
	ev_io_init (&bind->IO, io_cb, fd, EV_READ);
	ev_io_start (SRV(bind->srv)->main_loop, &bind->IO);

	return ret_ok;

error:
	/* The socket was already opened by _set_client
	 */
	http2d_socket_close (&bind->socket);
	return ret;
}


ret_t
http2d_bind_init_port (http2d_bind_t         *bind,
		       cuint_t                listen_queue,
		       bool                   ipv6,
		       http2d_server_token_t  token)
{
	ret_t ret;

	/* Init the port
	 */
#ifdef HAVE_IPV6
	if (ipv6) {
		ret = init_socket (bind, AF_INET6);
	} else
#endif
	{
		ret = ret_not_found;
	}

	if (ret != ret_ok) {
		ret = init_socket (bind, AF_INET);

		if (ret != ret_ok) {
			LOG_CRITICAL (HTTP2D_ERROR_BIND_COULDNT_BIND_PORT,
				      bind->port, getuid(), getgid());
			goto error;
		}
	}

	/* Listen
	 */
	ret = http2d_socket_listen (&bind->socket, listen_queue);
	if (ret != ret_ok) {
		goto error;
	}

	/* Build the strings
	 */
	ret = build_strings (bind, token);
	if (ret != ret_ok) {
		goto error;
	}

	return ret_ok;

error:
	http2d_socket_close (&bind->socket);
	return ret_error;
}


ret_t
http2d_bind_accept_more (http2d_bind_t *bind,
			 ret_t          prev_ret)
{
	/* Failed to accept
	 */
	if (prev_ret != ret_ok) {
		bind->accept_continuous = 0;

		if (bind->accept_recalculate)
			bind->accept_recalculate -= 1;
		else
			bind->accept_recalculate  = 10;

		return ret_deny;
	}

	/* Did accept a connection
	 */
	bind->accept_continuous++;

	if (bind->accept_recalculate <= 0) {
		bind->accept_continuous_max = bind->accept_continuous;
		return ret_ok;
	}

	if (bind->accept_continuous < bind->accept_continuous_max) {
		return ret_ok;
	}

	bind->accept_continuous = 0;
	return ret_deny;
}
