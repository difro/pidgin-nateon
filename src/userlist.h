/**
 * @file userlist.h NATEON user list support
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
#ifndef _NATEON_USERLIST_H_
#define _NATEON_USERLIST_H_

typedef struct _NateonUserList NateonUserList;

#include "cmdproc.h"
#include "user.h"
#include "group.h"

typedef enum
{
	NATEON_LIST_FL,
	NATEON_LIST_AL,
	NATEON_LIST_BL,
	NATEON_LIST_RL

} NateonListId;

typedef struct
{
	char *who;
	char *old_group_name;

} NateonMoveBuddy;

struct _NateonUserList
{
	NateonSession *session;

	/* NateonUsers *users; */
	/* NateonGroups *groups; */

	GList *users;
	GList *groups;

//	GQueue *buddy_icon_requests;
//	int buddy_icon_window;
//	guint buddy_icon_request_timer;
//
//	int fl_users_count;

};

NateonListId nateon_get_list_id(const char *list);

void nateon_got_add_user(NateonSession *session, NateonUser *user, NateonListId list_id, int group_id);
void nateon_got_rem_user(NateonSession *session, NateonUser *user, NateonListId list_id, int group_id);
void nateon_got_list_user(NateonSession *session, NateonUser *user, int list_op, GSList *group_ids);

NateonUserList *nateon_userlist_new(NateonSession *session);
void nateon_userlist_destroy(NateonUserList *userlist);
void nateon_userlist_add_user(NateonUserList *userlist, NateonUser *user);
//void nateon_userlist_remove_user(NateonUserList *userlist, NateonUser *user);
NateonUser *nateon_userlist_find_user_with_id(NateonUserList *userlist, const char *id);
NateonUser *nateon_userlist_find_user_with_name(NateonUserList *userlist, const char *account_name);
void nateon_userlist_add_group(NateonUserList *userlist, NateonGroup *group);
void nateon_userlist_remove_group(NateonUserList *userlist, NateonGroup *group);
NateonGroup *nateon_userlist_find_group_with_id(NateonUserList *userlist, int id);
NateonGroup *nateon_userlist_find_group_with_name(NateonUserList *userlist, const char *name);
int nateon_userlist_find_group_id(NateonUserList *userlist, const char *group_name);
const char *nateon_userlist_find_group_name(NateonUserList *userlist, int group_id);
void nateon_userlist_rename_group_id(NateonUserList *userlist, int group_id, const char *new_name);
void nateon_userlist_remove_group_id(NateonUserList *userlist, int group_id);

void nateon_userlist_rem_buddy(NateonUserList *userlist, const char *who, int list_id, const char *group_name);
void nateon_userlist_add_buddy(NateonUserList *userlist, const char *who, int list_id, const char *group_name);
void nateon_userlist_move_buddy(NateonUserList *userlist, const char *who, const char *old_group_name, const char *new_group_name);

#endif /* _NATEON_USERLIST_H_ */
