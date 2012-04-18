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

#include "config_node.h"
#include "config_reader.h"
#include "util.h"

#define ENTRIES "config"


/* Implements _new() and _free()
 */
HTTP2D_ADD_FUNC_NEW  (config_node);
HTTP2D_ADD_FUNC_FREE (config_node);


ret_t
http2d_config_node_init (http2d_config_node_t *conf)
{
	INIT_LIST_HEAD (&conf->entry);
	INIT_LIST_HEAD (&conf->child);

	http2d_buffer_init (&conf->key);
	http2d_buffer_init (&conf->val);

	return ret_ok;
}


ret_t
http2d_config_node_mrproper (http2d_config_node_t *conf)
{
	http2d_list_t *i, *j;

	http2d_buffer_mrproper (&conf->key);
	http2d_buffer_mrproper (&conf->val);

	list_for_each_safe (i, j, &conf->child) {
		http2d_config_node_free (CONFIG_NODE(i));
	}

	return ret_ok;
}


static http2d_config_node_t *
search_child (http2d_config_node_t *current, http2d_buffer_t *child)
{
	http2d_list_t        *i;
	http2d_config_node_t *entry;

	list_for_each (i, &current->child) {
		entry = CONFIG_NODE(i);

		if (strcmp (entry->key.buf, child->buf) == 0)
			return entry;
	}

	return NULL;
}


static http2d_config_node_t *
add_new_child (http2d_config_node_t *entry, http2d_buffer_t *key)
{
	ret_t                 ret;
	http2d_config_node_t *n;

	ret = http2d_config_node_new (&n);
	if (ret != ret_ok) return NULL;

	http2d_buffer_add_buffer (&n->key, key);

	http2d_list_add_tail (LIST(n), &entry->child);
	return n;
}


ret_t
http2d_config_node_add_buf (http2d_config_node_t *conf, http2d_buffer_t *key, http2d_buffer_t *val)
{
	char                 *sep;
	http2d_config_node_t *child;
	http2d_config_node_t *current = conf;
	const char           *begin   = key->buf;
	http2d_buffer_t       tmp     = HTTP2D_BUF_INIT;
	bool                  final   = false;

	/* 'include' is a special case
	 */
	if (http2d_buffer_cmp_str (key, "include") == 0) {
		return http2d_config_reader_parse (conf, val);
	}
	else if (http2d_buffer_cmp_str (key, "try_include") == 0) {
		http2d_config_reader_parse (conf, val);
	}

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			http2d_buffer_add (&tmp, begin, sep - begin);
		} else {
			http2d_buffer_add (&tmp, begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			child = add_new_child (current, &tmp);
			if (child == NULL) return ret_error;
		}

		if (final) {
			http2d_buffer_clean (&child->val);
			http2d_buffer_add_buffer (&child->val, val);
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		http2d_buffer_clean (&tmp);

	} while (! final);

	http2d_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t
http2d_config_node_get (http2d_config_node_t *conf, const char *key, http2d_config_node_t **entry)
{
	char                 *sep;
	http2d_config_node_t *child;
	http2d_config_node_t *current = conf;
	const char           *begin   = key;
	http2d_buffer_t       tmp     = HTTP2D_BUF_INIT;
	bool                  final   = false;

	do {
		/* Extract current child
		 */
		sep = strchr (begin, '!');
		if (sep != NULL) {
			http2d_buffer_add (&tmp, begin, sep - begin);
		} else {
			http2d_buffer_add (&tmp, begin, strlen(begin));
			final = true;
		}

		/* Look for the child entry
		 */
		child = search_child (current, &tmp);
		if (child == NULL) {
			http2d_buffer_mrproper (&tmp);
			return ret_not_found;
		}

		if (final) {
			*entry = child;
		}

		/* Prepare for next step
		 */
		begin = sep + 1;
		current = child;

		http2d_buffer_clean (&tmp);

	} while (! final);

	http2d_buffer_mrproper (&tmp);
	return ret_ok;
}


ret_t
http2d_config_node_get_buf (http2d_config_node_t *conf, http2d_buffer_t *key, http2d_config_node_t **entry)
{
	return http2d_config_node_get (conf, key->buf, entry);
}


ret_t
http2d_config_node_while (http2d_config_node_t *conf, http2d_config_node_while_func_t func, void *data)
{
	ret_t            ret;
	http2d_list_t *i;

	http2d_config_node_foreach (i, conf) {
		ret = func (CONFIG_NODE(i), data);
		if (ret != ret_ok) return ret;
	}

	return ret_ok;
}


ret_t
http2d_config_node_read (http2d_config_node_t *conf, const char *key, http2d_buffer_t **buf)
{
	ret_t                   ret;
	http2d_config_node_t *tmp;

	ret = http2d_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	*buf = &tmp->val;
	return ret_ok;
}


ret_t
http2d_config_node_copy (http2d_config_node_t *conf, const char *key, http2d_buffer_t *buf)
{
	ret_t              ret;
	http2d_buffer_t *tmp = NULL;

	ret = http2d_config_node_read (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	return http2d_buffer_add_buffer (buf, tmp);
}


ret_t
http2d_config_node_read_int (http2d_config_node_t *conf, const char *key, int *num)
{
	ret_t                   ret;
	http2d_config_node_t *tmp;

	ret = http2d_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	if (http2d_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	ret = http2d_atoi (tmp->val.buf, num);
	if (unlikely (ret != ret_ok))
		return ret_error;

	return ret_ok;
}


ret_t
http2d_config_node_read_long (http2d_config_node_t *conf, const char *key, long *num)
{
	ret_t                   ret;
	http2d_config_node_t *tmp;

	ret = http2d_config_node_get (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	if (http2d_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	*num = atol (tmp->val.buf);
	return ret_ok;
}


ret_t
http2d_config_node_read_bool (http2d_config_node_t *conf, const char *key, bool *val)
{
	ret_t ret;
	int   tmp;

	ret = http2d_config_node_read_int (conf, key, &tmp);
	if (ret != ret_ok) return ret;

	*val = !! tmp;
	return ret_ok;
}


ret_t
http2d_config_node_read_path (http2d_config_node_t *conf, const char *key, http2d_buffer_t **buf)
{
	ret_t                   ret;
	http2d_config_node_t *tmp;

	if (key != NULL) {
		ret = http2d_config_node_get (conf, key, &tmp);
		if (ret != ret_ok) return ret;
	} else {
		tmp = conf;
	}

	if (http2d_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	if (http2d_buffer_end_char (&tmp->val) != '/')
		http2d_buffer_add_str (&tmp->val, "/");

	*buf = &tmp->val;
	return ret_ok;
}


ret_t
http2d_config_node_read_list (http2d_config_node_t           *conf,
				const char                       *key,
				http2d_config_node_list_func_t  func,
				void                             *param)
{
	ret_t                   ret;
	char                   *ptr;
	char                   *stop;
	http2d_config_node_t *tmp;

	if (key != NULL) {
		ret = http2d_config_node_get (conf, key, &tmp);
		if (ret != ret_ok) return ret;
	} else {
		tmp = conf;
	}

	if (http2d_buffer_is_empty (&tmp->val)) {
		return ret_not_found;
	}

	ptr = tmp->val.buf;

	if (ptr == NULL)
		return ret_not_found;

	for (;;) {
		while ((*ptr == ' ') && (*ptr != '\0'))
			ptr++;

		stop = strchr (ptr, ',');
		if (stop != NULL) *stop = '\0';

		ret = func (ptr, param);
		if (ret != ret_ok) return ret;

		if (stop == NULL)
			break;

		*stop = ',';
		ptr = stop + 1;
	}

	return ret_ok;
}


static ret_t
convert_to_list_step (char *entry, void *data)
{
	HTTP2D_NEW(buf, buffer);

	http2d_buffer_add (buf, entry, strlen(entry));
	return http2d_list_add_tail_content (LIST(data), buf);
}


ret_t
http2d_config_node_convert_list (http2d_config_node_t *conf, const char *key, http2d_list_t *list)
{
	return http2d_config_node_read_list (conf, key, convert_to_list_step, list);
}
