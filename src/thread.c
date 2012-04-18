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
#include "thread.h"
#include "connection.h"


static void *
thread_main (void *param)
{
	ret_t            ret;
	int              params;
	http2d_list_t   *i, *j;
	http2d_thread_t *thd     = THREAD(param);

	while (true) {
		/* Process active connections
		 */
		list_for_each_safe (i, j, &thd->conns.active) {
			/* Update the pthread property */
			pthread_setspecific (thread_connection_ptr, i);

			/* Process connection */
			ret = http2d_connection_step (CONN(i));
			switch (ret) {
			case ret_ok:
				break;
			case ret_eof:
				http2d_thread_recycle_conn (thd, CONN(i));
				break;
			default:
				;
//				RET_UNKNOWN(ret);
			}
		}

		/* Calculate ev's loop parameters
		 */
		params = EVRUN_ONCE;
		if (! http2d_list_empty (&thd->conns.active)) {
			params |= EVRUN_NOWAIT;
		}

		/* Let ev check fds and execute callbacks
		 */
		ev_run (thd->loop, params);
	}
}


ret_t
http2d_thread_add_conn (http2d_thread_t     *thd,
			http2d_connection_t *conn)
{
	/* This function is only executed by bind.c
	 */
	pthread_mutex_lock (&thd->new_conns.mutex);
	{
		conn->thread = thd;
		conn->srv    = thd->srv;
		http2d_list_add (&conn->listed, &thd->new_conns.list);
	}
	pthread_mutex_unlock (&thd->new_conns.mutex);

	ev_async_send (thd->loop, &thd->new_conns.signal);
	return ret_ok;
}


static void
new_conn_cb (struct ev_loop *loop, ev_async *w, int revents)
{
	http2d_list_t   *i, *j;
	http2d_list_t   *conns;
	http2d_thread_t *thd    = list_entry (w, http2d_thread_t, new_conns.signal);

	/* Recently accepted connections are waiting
	 */
	pthread_mutex_lock (&thd->new_conns.mutex);
	{
		list_for_each_safe (i, j, &thd->new_conns.list) {
			/* Move to polling */
			http2d_connection_move_to_polling (CONN(i), EV_READ);
		}
	}
	pthread_mutex_unlock (&thd->new_conns.mutex);

	/* The loop shouldn't wait any longer
	 */
	ev_break (thd->loop, EVBREAK_ONE);
}


ret_t
http2d_thread_new (http2d_thread_t **thd,
		   void             *srv)
{
	int re;
	HTTP2D_NEW_STRUCT (n, thread);

	INIT_LIST_HEAD (&n->conns.active);
	INIT_LIST_HEAD (&n->conns.polling);

	/* Thread loop */
	n->loop = ev_loop_new (ev_recommended_backends() | EVBACKEND_KQUEUE | EVBACKEND_EPOLL | EVFLAG_NOENV);
	if (unlikely (n->loop == NULL))
		return ret_error;

	/* New connections */
 	INIT_LIST_HEAD (&n->new_conns.list);
	pthread_mutex_init (&n->new_conns.mutex, NULL);

	ev_async_init (&n->new_conns.signal, new_conn_cb);
	ev_async_start (n->loop, &n->new_conns.signal);

	/* Thread */
	re = pthread_create (&n->thread, NULL, thread_main, n);
	if (re) {
		return ret_error;
	}

	/* Properties */
	n->srv = srv;

	*thd = n;
	return ret_ok;
}


ret_t
http2d_thread_free (http2d_thread_t *thd)
{
	http2d_list_t *i, *j;

	list_for_each_safe (i, j, &thd->conns.active) {
		http2d_connection_free (CONN(i));
	}
	list_for_each_safe (i, j, &thd->conns.polling) {
		http2d_connection_free (CONN(i));
	}

	pthread_mutex_destroy (&thd->new_conns.mutex);
	pthread_cancel (thd->thread);
	ev_loop_destroy (thd->loop);

	free (thd);
	return ret_ok;
}


ret_t
http2d_thread_recycle_conn (http2d_thread_t     *thd,
			    http2d_connection_t *conn)
{
	/* Removed the connection is either the thread's active or
	 * polling lists.
	 */
	http2d_list_del (&conn->listed);

	// TODO: Implement connection recycle
	http2d_connection_free (conn);

	return ret_ok;
}
