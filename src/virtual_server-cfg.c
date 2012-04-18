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
#include "virtual_server.h"
#include "virtual_server-cfg.h"
#include "util.h"

#define ENTRIES "vserver,config"

static ret_t
configure_virtual_server_property (http2d_config_node_t *conf, void *data)
{
	ret_t                    ret;
	http2d_buffer_t         *tmp;
	http2d_virtual_server_t *vsrv = VSERVER(data);

	if (CONFIG_KEY_IS (conf, "ssl_certificate_file")) {
		http2d_buffer_add_buffer (&vsrv->ssl_certificate_file, &conf->val);

	} else if (CONFIG_KEY_IS (conf, "ssl_certificate_key_file")) {
		http2d_buffer_add_buffer (&vsrv->ssl_certificate_key_file, &conf->val);

	} else if (CONFIG_KEY_IS (conf, "ssl_ca_list_file")) {
		http2d_buffer_add_buffer (&vsrv->ssl_ca_list_file, &conf->val);

	} else if (CONFIG_KEY_IS (conf, "ssl_cipher_server_preference")) {
		ret = http2d_atob (conf->val.buf, &vsrv->ssl_cipher_server_preference);
		if (ret != ret_ok) return ret_error;

	} else if (CONFIG_KEY_IS (conf, "nick")) {
		http2d_buffer_add_buffer (&vsrv->name, &conf->val);

	} else if (CONFIG_KEY_IS (conf, "ssl_ciphers")) {
		http2d_buffer_clean (&vsrv->ssl_ciphers);
		http2d_buffer_add_buffer (&vsrv->ssl_ciphers, &conf->val);
	}

	return ret_ok;
}


ret_t
http2d_virtual_server_configure (http2d_virtual_server_t  *vsrv,
				 int                       prio,
				 http2d_config_node_t     *config)
{
	ret_t                 ret;
	int                   tmp;
	http2d_config_node_t *subconf;

	/* Set the priority
	 */
	vsrv->priority = prio;

	/* Is disabled?
	*/
	ret = http2d_config_node_get (config, "disabled", &subconf);
	if (ret == ret_ok) {
		ret = http2d_atoi (subconf->val.buf, &tmp);
		if ((ret == ret_ok) && (tmp)) {
			TRACE(ENTRIES, "Skipping VServer '%s'\n", config->key.buf);
			return ret_deny;
		}
	}

	/* Parse properties
	 */
	ret = http2d_config_node_while (config, configure_virtual_server_property, vsrv);
	if (ret != ret_ok) {
		return ret;
	}


	return ret_ok;
}
