/**
 * @file sync.h NATEON list synchronization functions
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
#ifndef _NATEON_SYNC_H_
#define _NATEON_SYNC_H_

typedef struct _NateonSync NateonSync;

#include "session.h"
#include "table.h"
//#include "user.h"

struct _NateonSync
{
	NateonSession *session;
	NateonTable *cbs_table;
	NateonTable *old_cbs_table;

	int num_users;
//	int total_users;
	int num_groups;
//	int total_groups;
	NateonUser *last_user;
};

void nateon_sync_init(void);
void nateon_sync_end(void);

NateonSync * nateon_sync_new(NateonSession *session);
void nateon_sync_destroy(NateonSync *sync);

#endif /* _NATEON_SYNC_H_ */
