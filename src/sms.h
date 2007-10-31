/**
 * @file sms.h Paging functions
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
#ifndef _NATEON_SMS_H_
#define _NATEON_SMS_H_

typedef struct _NateonSms NateonSms;

#include "session.h"

/**
 * A sms.
 */
struct _NateonSms
{
	char *from;
	char *to;

	char *body;
};

/**
 * Creates a new, empty sms.
 *
 * @return A new sms.
 */
NateonSms *nateon_sms_new(const char *from, const char *to);

/**
 * Destroys a sms.
 */
void nateon_sms_destroy(NateonSms *sms);

/**
 * Generates the payload data of a sms.
 *
 * @param sms     The sms.
 * @param ret_size The returned size of the payload.
 *
 * @return The payload data of a sms.
 */
char *nateon_sms_gen_payload(const NateonSms *sms, size_t *ret_size);

/**
 * Sets the body of a sms.
 *
 * @param sms  The sms.
 * @param body The body of the sms.
 */
void nateon_sms_set_body(NateonSms *sms, const char *body);

/**
 * Returns the body of the sms.
 *
 * @param sms The sms.
 *
 * @return The body of the sms.
 */
const char *nateon_sms_get_body(const NateonSms *sms);

#endif /* _NATEON_SMS_H_ */
