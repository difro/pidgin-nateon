/**
 * @file dialog.c Dialog functions
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
#include "dialog.h"

typedef struct
{
	PurpleConnection *gc;
	char *who;
	char *group;
	gboolean add;

} NateonAddRemData;

static void
nateon_add_cb(NateonAddRemData *data)
{
	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	if (g_list_find(purple_connections_get_all(), data->gc) != NULL)
	{
		NateonSession *session = data->gc->proto_data;
		NateonUserList *userlist = session->userlist;

		//nateon_userlist_add_buddy(userlist, data->who, NATEON_LIST_FL, data->group);
	}

	if (data->group != NULL)
		g_free(data->group);

	g_free(data->who);
	g_free(data);
}

static void
nateon_rem_cb(NateonAddRemData *data)
{
	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	if (g_list_find(purple_connections_get_all(), data->gc) != NULL)
	{
		NateonSession *session = data->gc->proto_data;
		NateonUserList *userlist = session->userlist;

		//nateon_userlist_rem_buddy(userlist, data->who, NATEON_LIST_FL, data->group);
	}

	if (data->group != NULL)
		g_free(data->group);

	g_free(data->who);
	g_free(data);
}

void
nateon_show_sync_issue(NateonSession *session, const char *account_name,
					const char *group_name)
{
	PurpleConnection *gc;
	PurpleAccount *account;
	NateonAddRemData *data;
	char *msg, *reason;
	PurpleBuddy *buddy;
	PurpleGroup *group = NULL;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	account = session->account;
	gc = purple_account_get_connection(account);

	data        = g_new0(NateonAddRemData, 1);
	data->who   = g_strdup(account_name);
	data->group = g_strdup(group_name);
	data->gc    = gc;

	msg = g_strdup_printf(_("Buddy list synchronization issue in %s (%s)"),
						  purple_account_get_username(account),
						  purple_account_get_protocol_name(account));

	if (group_name != NULL)
	{
		reason = g_strdup_printf(_("%s on the local list is "
								   "inside the group \"%s\" but not on "
								   "the server list. "
								   "Do you want this buddy to be added?"),
								 account_name, group_name);
	}
	else
	{
		reason = g_strdup_printf(_("%s is on the local list but "
								   "not on the server list. "
								   "Do you want this buddy to be added?"),
								 account_name);
	}

	purple_request_action(gc, NULL, msg, reason, PURPLE_DEFAULT_ACTION_NONE, 
						data, 2,
						_("Yes"), G_CALLBACK(nateon_add_cb),
						_("No"), G_CALLBACK(nateon_rem_cb));

	if (group_name != NULL)
		group = purple_find_group(group_name);

	if (group != NULL)
		buddy = purple_find_buddy_in_group(account, account_name, group);
	else
		buddy = purple_find_buddy(account, account_name);

	if (buddy != NULL)
		purple_blist_remove_buddy(buddy);

	g_free(reason);
	g_free(msg);
}
