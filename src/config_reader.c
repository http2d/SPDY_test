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
#include "config_reader.h"
#include "error_log.h"
#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define ENTRIES "config"


static ret_t
do_parse_file (http2d_config_node_t *conf, const char *file)
{
	ret_t            ret;
	http2d_buffer_t  buf = HTTP2D_BUF_INIT;

	ret = http2d_buffer_read_file (&buf, (char *)file);
	if (ret != ret_ok) goto error;

	ret = http2d_config_reader_parse_string (conf, &buf);
	if (ret != ret_ok) goto error;

	http2d_buffer_mrproper (&buf);
	return ret_ok;

error:
	http2d_buffer_mrproper (&buf);
	return ret_error;
}


static ret_t
do_include (http2d_config_node_t *conf, http2d_buffer_t *path)
{
	int         re;
	struct stat info;

	re = http2d_stat (path->buf, &info);
	if (re < 0) {
		LOG_CRITICAL (HTTP2D_ERROR_CONF_READ_ACCESS_FILE, path->buf);
		return ret_error;
	}

	if (S_ISREG(info.st_mode)) {
		return do_parse_file (conf, path->buf);

	} else if (S_ISDIR(info.st_mode)) {
		DIR              *dir;
		struct dirent    *entry;
		int               entry_len;

		dir = http2d_opendir (path->buf);
		if (dir == NULL) return ret_error;

		while ((entry = readdir(dir)) != NULL) {
			ret_t             ret;
			http2d_buffer_t full_new = HTTP2D_BUF_INIT;

			/* Ignore backup files
			 */
			entry_len = strlen(entry->d_name);
			if (unlikely (entry_len <= 0)) {
				continue;
			}

			if ((entry->d_name[0] == '.') ||
			    (entry->d_name[0] == '#') ||
			    (entry->d_name[entry_len-1] == '~'))
			{
				continue;
			}

			ret = http2d_buffer_add_va (&full_new, "%s/%s", path->buf, entry->d_name);
			if (unlikely (ret != ret_ok)) {
				http2d_buffer_mrproper (&full_new);
				return ret;
			}

			ret = do_parse_file (conf, full_new.buf);
			if (unlikely (ret != ret_ok)) {
				http2d_buffer_mrproper (&full_new);
				return ret;
			}

			http2d_buffer_mrproper (&full_new);
		}

		http2d_closedir (dir);
		return ret_ok;
	}

	SHOULDNT_HAPPEN;
	return ret_error;
}


static ret_t
check_config_node_sanity (http2d_config_node_t *conf)
{
	http2d_list_t *i, *j;
	int              re;

	/* Finds duplicate properties written in lower/upper-case
	 */
	http2d_config_node_foreach (i, conf)
	{
		http2d_config_node_foreach (j, conf)
		{
			if (i == j)
				continue;

			re = http2d_buffer_case_cmp_buf (&CONFIG_NODE(i)->key,
							   &CONFIG_NODE(j)->key);
			if (re == 0) {
				LOG_ERROR (HTTP2D_ERROR_CONF_READ_CHILDREN_SAME_NODE,
					   CONFIG_NODE(i)->key.buf, CONFIG_NODE(j)->key.buf);
				return ret_error;
			}
		}
	}

	return ret_ok;
}


static int
cmp_num (http2d_list_t *a, http2d_list_t *b)
{
	int an = strtol(CONFIG_NODE(a)->key.buf, NULL, 10);
	int bn = strtol(CONFIG_NODE(b)->key.buf, NULL, 10);

	if (an == bn) return 0;
	if (an < bn)  return bn - an;

	return - (an - bn);
}

static ret_t
sort_lists (http2d_config_node_t *conf)
{
	int            re;
	ret_t          ret;
	http2d_list_t *i;
	bool           all_num = true;

	/* Are all numbers
	 */
	http2d_config_node_foreach (i, conf) {
		errno = 0;
		re    = strtol(CONFIG_NODE(i)->key.buf, NULL, 10);

		if ((re == 0) && (errno = EINVAL)) {
			all_num = false;
			break;
		}
	}

	/* Reorder entries
	 */
	if (all_num) {
		http2d_list_sort (&conf->child, cmp_num);
	}

	/* Iterate through the children
	 */
	http2d_config_node_foreach (i, conf) {
		ret = sort_lists (CONFIG_NODE(i));
		if (ret != ret_ok) {
			return ret;
		}
	}

	return ret_ok;
}


ret_t
http2d_config_reader_parse_string (http2d_config_node_t *conf, http2d_buffer_t *buf)
{
	ret_t            ret;
	char            *eol;
	char            *begin;
	char            *equal;
	char            *tmp;
	char            *eof;
	http2d_buffer_t  key    = HTTP2D_BUF_INIT;
	http2d_buffer_t  val    = HTTP2D_BUF_INIT;

	eof = buf->buf + buf->len;

	begin = buf->buf;
	do {
		/* Skip whites at the begining
		 */
		while ((begin < eof) &&
		       ((*begin == ' ') || (*begin == '\t') ||
			(*begin == '\r') || (*begin == '\n')))
		{
			begin++;
		}

		/* Look for the EOL
		 */
		eol = http2d_min_str (strchr(begin, '\n'),
				      strchr(begin, '\r'));

		if (eol == NULL) {
			eol = eof;
		}

		/* Check that it's long enough
		 */
		if (eol - begin <= 4) {
			goto next;
		}
		*eol = '\0';

		/* Read the line
		 */
		if (*begin != '#') {
			cuint_t val_len;

			equal = strstr (begin, " = ");
			if (equal == NULL) goto error;

			tmp = equal;

			/* Skip whites: end of the key
			 */
			while (*tmp == ' ')
				tmp--;
			http2d_buffer_add (&key, begin, (tmp + 1) - begin);

			tmp = equal + 3;
			while (*tmp == ' ')
				tmp++;

			/* Skip whites: end of the value
			 */
			val_len = strlen(tmp);
			while ((val_len >= 1) &&
			       (tmp[val_len-1] == ' '))
			{
				val_len--;
			}

			http2d_buffer_add (&val, tmp, val_len);

			TRACE(ENTRIES, "'%s' => '%s'\n", key.buf, val.buf);

			ret = http2d_config_node_add_buf (conf, &key, &val);
			if (ret != ret_ok)
				goto error;
		}

		/* Next loop
		 */
	next:
		begin = eol + 1;
		if (begin >= eof) {
			break;
		}

		http2d_buffer_clean (&key);
		http2d_buffer_clean (&val);

	} while (eol != NULL);

	http2d_buffer_mrproper (&key);
	http2d_buffer_mrproper (&val);

	/* Sort internal lists
	 */
	ret = sort_lists (conf);
	if (ret != ret_ok)
		return ret;

	/* Sanity check
	 */
	ret = check_config_node_sanity (conf);
	if (ret != ret_ok)
		return ret;

	return ret_ok;

error:
	LOG_ERROR (HTTP2D_ERROR_CONF_READ_PARSE, begin);

	http2d_buffer_mrproper (&key);
	http2d_buffer_mrproper (&val);
	return ret_error;
}


ret_t
http2d_config_reader_parse (http2d_config_node_t *conf, http2d_buffer_t *path)
{
	return do_include (conf, path);
}
