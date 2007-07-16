/**
 * @file history.h NATEON history functions
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
#ifndef _NATEON_HISTORY_H
#define _NATEON_HISTORY_H

#define NATEON_HIST_ELEMS 0x30

typedef struct _NateonHistory NateonHistory;

#include "transaction.h"

/**
 * The history.
 */
struct _NateonHistory
{
	GQueue *queue;
	unsigned int trId;
};

NateonHistory *nateon_history_new(void);
void nateon_history_destroy(NateonHistory *history);
NateonTransaction *nateon_history_find(NateonHistory *history, unsigned int triId);
void nateon_history_add(NateonHistory *history, NateonTransaction *trans);

#endif /* _NATEON_HISTORY_H */
