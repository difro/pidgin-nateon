/**
 * @file state.h State functions and definitions
 * @ingroup nateon 
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
#ifndef _NATEON_STATE_H_
#define _NATEON_STATE_H_

/**
 * Away types.
 */
typedef enum
{
	NATEON_ONLINE  = 0,
	NATEON_AWAY    = 1,
	NATEON_BUSY    = 2,
	NATEON_PHONE   = 3,
	NATEON_MEETING = 4,
	NATEON_OFFLINE = 5,
	NATEON_HIDDEN  = 6,
	NATEON_IDLE    = 7

} NateonAwayType;

/**
 * Changes the status of the user.
 *
 * @param session The NATEON session.
 */
void nateon_change_status(NateonSession *session);

/**
 * Returns the string representation of an away type.
 *
 * @param type The away type.
 *
 * @return The string representation of the away type.
 */
//const char *nateon_away_get_text(NateonAwayType type);

const char *nateon_state_get_text(NateonAwayType state);

NateonAwayType nateon_state_from_account(PurpleAccount *account);

#endif /* _NATEON_STATE_H_ */
