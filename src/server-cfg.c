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
#include "virtual_server.h"
#include "virtual_server-cfg.h"
#include "bind.h"
#include "error_log.h"
#include "util.h"

#define ENTRIES "server,config"


static ret_t
configure_binds (http2d_server_t      *srv,
		 http2d_config_node_t *conf)
{
	ret_t          ret;
	http2d_list_t *i;
	http2d_bind_t *bind;

	/* Configure
	 */
	list_for_each (i, &conf->child) {
		ret = http2d_bind_new (&bind, srv);
		if (ret != ret_ok)
			return ret;

		ret = http2d_bind_configure (bind, CONFIG_NODE(i));
		if (ret != ret_ok) {
			http2d_bind_free (bind);
			return ret;
		}

		http2d_list_add_tail (&bind->listed, &srv->binds);
	}

	return ret_ok;
}


static ret_t
configure_server_property (http2d_config_node_t *conf,
			   void                 *data)
{
	ret_t              ret;
	int                val;
	char              *key = conf->key.buf;
	http2d_server_t *srv = SRV(data);

	if (CONFIG_KEY_IS (conf, "bind")) {
		ret = configure_binds (srv, conf);
		if (ret != ret_ok)
			return ret;
	} else {
		LOG_CRITICAL (HTTP2D_ERROR_SERVER_PARSE, key);
		return ret_error;
	}

	return ret_ok;
}


static ret_t
post_config_checks (http2d_server_t *srv)
{
	if (http2d_list_empty (&srv->vservers)) {
		LOG_CRITICAL_S (HTTP2D_ERROR_SERVER_NO_VSERVERS);
		return ret_error;
	}

	if (http2d_list_empty (&srv->binds)) {
		return ret_error;
	}

	return ret_ok;
}


static int
vserver_cmp (http2d_list_t *a, http2d_list_t *b)
{
	return (VSERVER(b)->priority - VSERVER(a)->priority);
}


static ret_t
add_vserver (http2d_config_node_t *conf, void *data)
{
	ret_t                    ret;
	cint_t                   prio = -1;
	http2d_virtual_server_t *vsrv = NULL;
 	http2d_server_t         *srv  = SRV(data);

	ret = http2d_atoi (conf->key.buf, &prio);
	if ((ret != ret_ok) || (prio < 0)) {
		LOG_CRITICAL (HTTP2D_ERROR_SERVER_VSERVER_PRIO, conf->key.buf);
		return ret_error;
	}

	TRACE (ENTRIES, "Adding vserver prio=%d\n", prio);

	/* Create a new vserver and enqueue it
	 */
	ret = http2d_virtual_server_new (&vsrv, srv);
	if (ret != ret_ok)
		return ret;

	/* Configure the new object
	 */
	ret = http2d_virtual_server_configure (vsrv, prio, conf);
	if (ret == ret_deny) {
		http2d_virtual_server_free (vsrv);
		return ret_ok;
	} else if (ret != ret_ok) {
		return ret;
	}

	ret = http2d_virtual_server_initialize (vsrv);
	if (ret != ret_ok) {
		http2d_virtual_server_free (vsrv);
		return ret_error;
	}

	/* Add it to the list
	 */
	http2d_list_add_tail (LIST(vsrv), &srv->vservers);
	return ret_ok;
}


ret_t
_http2d_server_config (http2d_server_t      *srv,
		       http2d_config_node_t *config)
{
	ret_t                 ret;
	http2d_config_node_t *subconf, *subconf2;

	/* Server
	 */
	TRACE (ENTRIES, "Configuring %s\n", "server");
	ret = http2d_config_node_get (config, "server", &subconf);
	if (ret == ret_ok) {
		/* Server properties
		 */
		ret = http2d_config_node_while (subconf, configure_server_property, srv);
		if (ret != ret_ok)
			return ret;
	}

	/* Load the virtual servers
	 */
	TRACE (ENTRIES, "Configuring %s\n", "virtual servers");
	ret = http2d_config_node_get (&srv->config, "vserver", &subconf);
	if (ret == ret_ok) {
		ret = http2d_config_node_while (subconf, add_vserver, srv);
		if (ret != ret_ok) return ret;
	}

	http2d_list_sort (&srv->vservers, vserver_cmp);

	/* Post configuration checks
	 */
	ret = post_config_checks (srv);
	if (unlikely (ret != ret_ok)) {
		return ret_error;
	}

	return ret_ok;
}
