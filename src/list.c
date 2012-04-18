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
#include "list.h"

#include <stdlib.h>

ret_t
http2d_list_get_len (http2d_list_t *head, size_t *len)
{
	http2d_list_t *i;
	cuint_t          n = 0;

	list_for_each (i, head)
		n++;

	*len = n;
	return ret_ok;
}


void
http2d_list_sort (http2d_list_t *head, int (*cmp)(http2d_list_t *a, http2d_list_t *b))
{
	http2d_list_t *p, *q, *e, *list, *tail, *oldhead;
	int insize, nmerges, psize, qsize, i;

	list = head->next;
	http2d_list_del(head);
	insize = 1;
	for (;;) {
		p = oldhead = list;
		list = tail = NULL;
		nmerges = 0;

		while (p) {
			nmerges++;
			q = p;
			psize = 0;
			for (i = 0; i < insize; i++) {
				psize++;
				q = q->next == oldhead ? NULL : q->next;
				if (!q)
					break;
			}

			qsize = insize;
			while (psize > 0 || (qsize > 0 && q)) {
				if (!psize) {
					e = q;
					q = q->next;
					qsize--;
					if (q == oldhead)
						q = NULL;
				} else if (!qsize || !q) {
					e = p;
					p = p->next;
					psize--;
					if (p == oldhead)
						p = NULL;
				} else if (cmp(p, q) <= 0) {
					e = p;
					p = p->next;
					psize--;
					if (p == oldhead)
						p = NULL;
				} else {
					e = q;
					q = q->next;
					qsize--;
					if (q == oldhead)
						q = NULL;
				}
				if (tail)
					tail->next = e;
				else
					list = e;
				e->prev = tail;
				tail = e;
			}
			p = q;
		}

		tail->next = list;
		list->prev = tail;

		if (nmerges <= 1)
			break;

		insize *= 2;
	}

	head->next = list;
	head->prev = list->prev;
	list->prev->next = head;
	list->prev = head;
}


ret_t
http2d_list_invert (http2d_list_t *head)
{
	http2d_list_t *i, *tmp;
	http2d_list_t new_list;

	INIT_LIST_HEAD (&new_list);

	list_for_each_safe (i, tmp, head) {
		http2d_list_add (i, &new_list);
	}

	http2d_list_reparent (&new_list, head);
	return ret_ok;
}


ret_t
http2d_list_add_content (http2d_list_t *head, void *item)
{
	HTTP2D_NEW_STRUCT(n,list_item);

	/* Init
	 */
	INIT_LIST_HEAD (LIST(n));
	n->info = item;

	/* Add to list
	 */
	http2d_list_add (LIST(n), head);

	return ret_ok;
}


ret_t
http2d_list_add_tail_content (http2d_list_t *head, void *item)
{
	HTTP2D_NEW_STRUCT(n,list_item);

	/* Init
	 */
	INIT_LIST_HEAD (LIST(n));
	n->info = item;

	/* Add to list
	 */
	http2d_list_add_tail (LIST(n), head);

	return ret_ok;
}


ret_t
http2d_list_content_free (http2d_list_t *head, http2d_list_free_func free_func)
{
	http2d_list_t *i, *tmp;

	list_for_each_safe (i, tmp, head) {
		http2d_list_content_free_item (i, free_func);
	}

	INIT_LIST_HEAD(head);

	return ret_ok;
}


ret_t
http2d_list_content_free_item (http2d_list_t *head, http2d_list_free_func free_func)
{
	http2d_list_del (head);

	if ((free_func != NULL) && (LIST_ITEM(head)->info)) {
		free_func (LIST_ITEM(head)->info);
	}

	free (head);
	return ret_ok;
}


ret_t
http2d_list_content_free_item_simple (http2d_list_t *head)
{
	http2d_list_del (head);

	if (LIST_ITEM(head)->info) {
		free (LIST_ITEM(head)->info);
	}

	free (head);
	return ret_ok;
}
