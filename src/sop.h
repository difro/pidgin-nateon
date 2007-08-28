/**
 * @file sop.h Paging functions
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
#ifndef _NATEON_SOP_H_
#define _NATEON_SOP_H_

typedef struct _NateonSop NateonSop;

#include "session.h"

/**
 * A sop.
 */
struct _NateonSop
{
	char *from;
	char *to;

	char *body;
};

/**
 * Creates a new, empty sop.
 *
 * @return A new sop.
 */
NateonSop *nateon_sop_new(const char *from, const char *to);

/**
 * Destroys a sop.
 */
void nateon_sop_destroy(NateonSop *sop);

/**
 * Generates the payload data of a sop.
 *
 * @param sop     The sop.
 * @param ret_size The returned size of the payload.
 *
 * @return The payload data of a sop.
 */
char *nateon_sop_gen_payload(const NateonSop *sop, size_t *ret_size);

/**
 * Sets the body of a sop.
 *
 * @param sop  The sop.
 * @param body The body of the sop.
 */
void nateon_sop_set_body(NateonSop *sop, const char *body);

/**
 * Returns the body of the sop.
 *
 * @param sop The sop.
 *
 * @return The body of the sop.
 */
const char *nateon_sop_get_body(const NateonSop *sop);

#endif /* _NATEON_SOP_H_ */
