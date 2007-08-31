/**
 * @file history.c NATEON history functions
 *
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "nateon.h"
#include "history.h"

NateonHistory *
nateon_history_new(void)
{
	NateonHistory *history = g_new0(NateonHistory, 1);

	history->trId = 1;

	history->queue = g_queue_new();

	return history;
}

void
nateon_history_destroy(NateonHistory *history)
{
	NateonTransaction *trans;

	while ((trans = g_queue_pop_head(history->queue)) != NULL)
		nateon_transaction_destroy(trans);

	g_queue_free(history->queue);
	g_free(history);
}

NateonTransaction *
nateon_history_find(NateonHistory *history, unsigned int trId)
{
	NateonTransaction *trans;
	GList *list;

	for (list = history->queue->head; list != NULL; list = list->next)
	{
		trans = list->data;
		if (trans->trId == trId)
			return trans;
	}

	return NULL;
}

void
nateon_history_add(NateonHistory *history, NateonTransaction *trans)
{
	GQueue *queue;

	g_return_if_fail(history != NULL);
	g_return_if_fail(trans   != NULL);

	queue = history->queue;

	trans->trId = history->trId++;

	g_queue_push_tail(queue, trans);

	if (queue->length > NATEON_HIST_ELEMS)
	{
		trans = g_queue_pop_head(queue);
		nateon_transaction_destroy(trans);
	}
}
