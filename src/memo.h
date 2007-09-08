/**
 * @file memo.h Paging functions
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
#ifndef _NATEON_MEMO_H_
#define _NATEON_MEMO_H_

typedef struct _NateonMemo NateonMemo;

#include "session.h"

/**
 * A memo.
 */
struct _NateonMemo
{
	char *from;
	char *to;

	char *body;
};

/**
 * Creates a new, empty memo.
 *
 * @return A new memo.
 */
NateonMemo *nateon_memo_new(const char *from, const char *to);

/**
 * Destroys a memo.
 */
void nateon_memo_destroy(NateonMemo *memo);

/**
 * Generates the payload data of a memo.
 *
 * @param memo     The memo.
 * @param ret_size The returned size of the payload.
 *
 * @return The payload data of a memo.
 */
char *nateon_memo_gen_payload(const NateonMemo *memo, size_t *ret_size);

/**
 * Sets the body of a memo.
 *
 * @param memo  The memo.
 * @param body The body of the memo.
 */
void nateon_memo_set_body(NateonMemo *memo, const char *body);

/**
 * Returns the body of the memo.
 *
 * @param memo The memo.
 *
 * @return The body of the memo.
 */
const char *nateon_memo_get_body(const NateonMemo *memo);

#endif /* _NATEON_MEMO_H_ */
