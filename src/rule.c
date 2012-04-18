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
#include "rule.h"
#include "connection.h"
#include "virtual_server.h"
#include "util.h"

#define ENTRIES "rule"

ret_t
http2d_rule_init_base (http2d_rule_t *rule, http2d_plugin_info_t *info)
{
	http2d_module_init_base (MODULE(rule), NULL, info);
	INIT_LIST_HEAD (&rule->list_node);

	rule->match       = NULL;
	rule->final       = true;
	rule->priority    = HTTP2D_RULE_PRIO_NONE;
	rule->parent_rule = NULL;

	http2d_config_entry_init (&rule->config);
	return ret_ok;
}


ret_t
http2d_rule_free (http2d_rule_t *rule)
{
	/* Free the Config Entry property
	 */
	http2d_config_entry_mrproper (&rule->config);

	/* Call the virtual method
	 */
	if (MODULE(rule)->free) {
		MODULE(rule)->free (rule);
	}

	/* Free the rule
	 */
	free (rule);
	return ret_ok;
}


static ret_t
configure_base (http2d_rule_t           *rule,
		http2d_config_node_t    *conf)
{
	ret_t              ret;
	http2d_buffer_t *final = NULL;

	/* Set the final value
	 */
	ret = http2d_config_node_read (conf, "final", &final);
	if (ret == ret_ok) {
		ret = http2d_atob (final->buf, &rule->final);
		if (ret != ret_ok) return ret_error;

		TRACE(ENTRIES, "Rule prio=%d set final to %d\n",
		      rule->priority, rule->final);
	}

	return ret_ok;
}


ret_t
http2d_rule_configure (http2d_rule_t *rule, http2d_config_node_t *conf, void *vsrv)
{
	ret_t ret;

	return_if_fail (rule, ret_error);
	return_if_fail (rule->configure, ret_error);

	ret = configure_base (rule, conf);
	if (ret != ret_ok) return ret;

	/* Call the real method
	 */
	return rule->configure (rule, conf, VSERVER(vsrv));
}


ret_t
http2d_rule_match (http2d_rule_t *rule, void *cnt, void *ret_conf)
{
	return_if_fail (rule, ret_error);
	return_if_fail (rule->match, ret_error);

	/* Call the real method
	 */
	return rule->match (rule, CONN(cnt), CONF_ENTRY(ret_conf));
}


void
http2d_rule_get_config (http2d_rule_t          *rule,
			  http2d_config_entry_t **config)
{
	http2d_rule_t *r = rule;

	while (r->parent_rule != NULL) {
		r = r->parent_rule;
	}

	*config = &r->config;
}
