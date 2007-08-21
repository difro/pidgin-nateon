/**
 * @file userlist.c NATEON user list support
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
#include "userlist.h"

const char *lists[] = { "FL", "AL", "BL", "RL" };

typedef struct
{
	PurpleConnection *gc;
	char *who;
	char *friendly;

} NateonPermitAdd;

/**************************************************************************
 * Callbacks
 **************************************************************************/
//static void
//nateon_accept_add_cb(NateonPermitAdd *pa)
//{
//	if (g_list_find(purple_connections_get_all(), pa->gc) != NULL)
//	{
//		NateonSession *session = pa->gc->proto_data;
//		NateonUserList *userlist = session->userlist;
//		PurpleBuddy *buddy;
//
//		//nateon_userlist_add_buddy(userlist, pa->who, NATEON_LIST_AL, NULL);
//		//nateon_userlist_add_buddy(userlist, pa->who, 0, NULL);
//
//		buddy = purple_find_buddy(pa->gc->account, pa->who);
//
//		if (buddy != NULL)
//			purple_account_notify_added(pa->gc->account, pa->who,
//				NULL, pa->friendly, NULL);
//		else
//			purple_account_request_add(pa->gc->account, pa->who,
//				NULL, pa->friendly, NULL);
//	}
//
//	g_free(pa->who);
//	g_free(pa->friendly);
//	g_free(pa);
//}
//
//static void
//nateon_cancel_add_cb(NateonPermitAdd *pa)
//{
//	if (g_list_find(purple_connections_get_all(), pa->gc) != NULL)
//	{
//		NateonSession *session = pa->gc->proto_data;
//		NateonUserList *userlist = session->userlist;
//
//		//nateon_userlist_add_buddy(userlist, pa->who, NATEON_LIST_BL, NULL);
//	}
//
//	g_free(pa->who);
//	g_free(pa->friendly);
//	g_free(pa);
//}
//
//static void
//got_new_entry(PurpleConnection *gc, const char *account_name, const char *friendly)
//{
//	NateonPermitAdd *pa;
//	char *msg;
//
//	pa = g_new0(NateonPermitAdd, 1);
//	pa->who = g_strdup(account_name);
//	pa->friendly = g_strdup(friendly);
//	pa->gc = gc;
//
//	if (friendly != NULL)
//	{
//		msg = g_strdup_printf(
//				   _("The user %s (%s) wants to add %s to his or her "
//					 "buddy list."),
//				   account_name, friendly,
//				   purple_account_get_username(gc->account));
//	}
//	else
//	{
//		msg = g_strdup_printf(
//				   _("The user %s wants to add %s to his or "
//					 "her buddy list."),
//				   account_name, purple_account_get_username(gc->account));
//	}
//
//	purple_request_action(gc, NULL, msg, NULL,
//						PURPLE_DEFAULT_ACTION_NONE, pa, 2,
//						_("Authorize"), G_CALLBACK(nateon_accept_add_cb),
//						_("Deny"), G_CALLBACK(nateon_cancel_add_cb));
//
//	g_free(msg);
//}

/**************************************************************************
 * Utility functions
 **************************************************************************/

static gboolean
user_is_in_group(NateonUser *user, int group_id)
{
	if (user == NULL)
		return FALSE;

	if (group_id < 0)
		return FALSE;

	if (g_list_find(user->group_ids, GINT_TO_POINTER(group_id)))
		return TRUE;

	return FALSE;
}

static gboolean
user_is_there(NateonUser *user, int list_id, int group_id)
{
	int list_op;

	if (user == NULL)
		return FALSE;

	list_op = 1 << list_id;

	if (!(user->list_op & list_op))
		return FALSE;

	if (list_id == NATEON_LIST_FL)
	{
		if (group_id >= 0)
			return user_is_in_group(user, group_id);
	}

	return TRUE;
}

//static const char*
//get_store_name(NateonUser *user)
//{
//	const char *store_name;
//
//	g_return_val_if_fail(user != NULL, NULL);
//
//	store_name = nateon_user_get_store_name(user);
//
//	if (store_name != NULL)
//		store_name = purple_url_encode(store_name);
//	else
//		store_name = nateon_user_get_account_name(user);
//
//	/* this might be a bit of a hack, but it should prevent notification server
//	 * disconnections for people who have buddies with insane friendly names
//	 * who added you to their buddy list from being disconnected. Stu. */
//	/* Shx: What? Isn't the store_name obtained from the server, and hence it's
//	 * below the BUDDY_ALIAS_MAXLEN ? */
//	/* Stu: yeah, that's why it's a bit of a hack, as you pointed out, we're
//	 * probably decoding the incoming store_name wrong, or something. bleh. */
//
//	if (strlen(store_name) > BUDDY_ALIAS_MAXLEN)
//		store_name = nateon_user_get_account_name(user);
//
//	return store_name;
//}

static void
nateon_request_add_group(NateonUserList *userlist, const char *who,
					  const char *old_group_name, const char *new_group_name)
{
	NateonCmdProc *cmdproc;
	NateonTransaction *trans;
	NateonMoveBuddy *data;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	cmdproc = userlist->session->notification->cmdproc;
	data = g_new0(NateonMoveBuddy, 1);

	data->who = g_strdup(who);

	if (old_group_name)
		data->old_group_name = g_strdup(old_group_name);

	trans = nateon_transaction_new(cmdproc, "ADDG", "%d %s", 0, purple_strreplace(new_group_name, " ", "%20"));

	nateon_transaction_set_data(trans, data);

	nateon_cmdproc_send_trans(cmdproc, trans);
}

///**************************************************************************
// * Server functions
// **************************************************************************/
//
//NateonListId
//nateon_get_list_id(const char *list)
//{
//	if (list[0] == 'F')
//		return NATEON_LIST_FL;
//	else if (list[0] == 'A')
//		return NATEON_LIST_AL;
//	else if (list[0] == 'B')
//		return NATEON_LIST_BL;
//	else if (list[0] == 'R')
//		return NATEON_LIST_RL;
//
//	return -1;
//}
//
//void
//nateon_got_add_user(NateonSession *session, NateonUser *user,
//				 NateonListId list_id, int group_id)
//{
//	PurpleAccount *account;
//	const char *account_name;
//	const char *friendly;
//
//	account = session->account;
//
//	account_name = nateon_user_get_account_name(user);
//	friendly = nateon_user_get_friendly_name(user);
//
//	if (list_id == NATEON_LIST_FL)
//	{
//		PurpleConnection *gc;
//
//		gc = purple_account_get_connection(account);
//
//		serv_got_alias(gc, account_name, friendly);
//
//		if (group_id >= 0)
//		{
//			nateon_user_add_group_id(user, group_id);
//		}
//		else
//		{
//			/* session->sync->fl_users_count++; */
//		}
//	}
//	else if (list_id == NATEON_LIST_AL)
//	{
//		purple_privacy_permit_add(account, account_name, TRUE);
//	}
//	else if (list_id == NATEON_LIST_BL)
//	{
//		purple_privacy_deny_add(account, account_name, TRUE);
//	}
//	else if (list_id == NATEON_LIST_RL)
//	{
//		PurpleConnection *gc;
//		PurpleConversation *convo;
//
//		gc = purple_account_get_connection(account);
//
//		purple_debug_info("nateon",
//						"%s has added you to his or her buddy list.\n",
//						account_name);
//
// 		convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, account_name, account);
// 		if (convo) {
// 			PurpleBuddy *buddy;
// 			char *msg;
// 
// 			buddy = purple_find_buddy(account, account_name);
// 			msg = g_strdup_printf(
// 				_("%s has added you to his or her buddy list."),
// 				buddy ? purple_buddy_get_contact_alias(buddy) : account_name);
// 			purple_conv_im_write(PURPLE_CONV_IM(convo), account_name, msg,
// 				PURPLE_MESSAGE_SYSTEM, time(NULL));
// 			g_free(msg);
// 		}
// 
//		if (!(user->list_op & (NATEON_LIST_AL_OP | NATEON_LIST_BL_OP)))
//		{
//			got_new_entry(gc, account_name, friendly);
//		}
//	}
//
//	user->list_op |= (1 << list_id);
//	/* purple_user_add_list_id (user, list_id); */
//}
//
//void
//nateon_got_rem_user(NateonSession *session, NateonUser *user,
//				 NateonListId list_id, int group_id)
//{
//	PurpleAccount *account;
//	const char *account_name;
//
//	account = session->account;
//
//	account_name = nateon_user_get_account_name(user);
//
//	if (list_id == NATEON_LIST_FL)
//	{
//		/* TODO: When is the user totally removed? */
//		if (group_id >= 0)
//		{
//			nateon_user_remove_group_id(user, group_id);
//			return;
//		}
//		else
//		{
//			/* session->sync->fl_users_count--; */
//		}
//	}
//	else if (list_id == NATEON_LIST_AL)
//	{
//		purple_privacy_permit_remove(account, account_name, TRUE);
//	}
//	else if (list_id == NATEON_LIST_BL)
//	{
//		purple_privacy_deny_remove(account, account_name, TRUE);
//	}
//	else if (list_id == NATEON_LIST_RL)
//	{
//		PurpleConversation *convo;
//
//		purple_debug_info("nateon",
//						"%s has removed you from his or her buddy list.\n",
//						account_name);
//
//		convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, account_name, account);
//		if (convo) {
//			PurpleBuddy *buddy;
//			char *msg;
//
//			buddy = purple_find_buddy(account, account_name);
//			msg = g_strdup_printf(
//				_("%s has removed you from his or her buddy list."),
//				buddy ? purple_buddy_get_contact_alias(buddy) : account_name);
//			purple_conv_im_write(PURPLE_CONV_IM(convo), account_name, msg,
//				PURPLE_MESSAGE_SYSTEM, time(NULL));
//			g_free(msg);
//		}
//	}
//
//	user->list_op &= ~(1 << list_id);
//	/* purple_user_remove_list_id (user, list_id); */
//
//	if (user->list_op == 0)
//	{
//		purple_debug_info("nateon", "Buddy '%s' shall be deleted?.\n",
//						account_name);
//
//	}
//}

void
nateon_got_list_user(NateonSession *session, NateonUser *user, int list_op, GSList *group_ids)
{
	PurpleConnection *gc;
	PurpleAccount *account;
	const char *account_name;
	const char *store;

	account = session->account;
	gc = purple_account_get_connection(account);

	account_name = nateon_user_get_account_name(user);
	store = nateon_user_get_store_name(user);

	if (list_op & NATEON_LIST_FL_OP)
	{
		GList *c;
		for (c = group_ids; c != NULL; c = c->next)
		{
			int group_id;
			group_id = GPOINTER_TO_INT(c->data);
			nateon_user_add_group_id(user, group_id);
		}

		/* FIXME: It might be a real alias */
		/* Umm, what? This might fix bug #1385130 */
		serv_got_alias(gc, account_name, store);
	}
//
//	if (list_op & NATEON_LIST_AL_OP)
//	{
//		/* These are users who are allowed to see our status. */
//
//		if (g_slist_find_custom(account->deny, account_name,
//								(GCompareFunc)strcmp))
//		{
//			purple_privacy_deny_remove(gc->account, account_name, TRUE);
//		}
//
//		purple_privacy_permit_add(account, account_name, TRUE);
//	}

//	if (list_op & NATEON_LIST_BL_OP)
//	{
//		/* These are users who are not allowed to see our status. */
//
//		if (g_slist_find_custom(account->permit, account_name,
//								(GCompareFunc)strcmp))
//		{
//			purple_privacy_permit_remove(gc->account, account_name, TRUE);
//		}
//
//		purple_privacy_deny_add(account, account_name, TRUE);
//	}
//
//	if (list_op & NATEON_LIST_RL_OP)
//	{
//		/* These are users who have us on their buddy list. */
//		/* TODO: what does store name is when this happens? */
//
//		if (!(list_op & (NATEON_LIST_AL_OP | NATEON_LIST_BL_OP)))
//		{
//			got_new_entry(gc, account_name, store);
//		}
//	}

	user->list_op = list_op;
}

/**************************************************************************
 * UserList functions
 **************************************************************************/

NateonUserList*
nateon_userlist_new(NateonSession *session)
{
	NateonUserList *userlist;

	userlist = g_new0(NateonUserList, 1);

	userlist->session = session;
//	userlist->buddy_icon_requests = g_queue_new();
	
//	/* buddy_icon_window is the number of allowed simultaneous buddy icon requests.
//	 * XXX With smarter rate limiting code, we could allow more at once... 5 was the limit set when
//	 * we weren't retrieiving any more than 5 per NATEON session. */
//	userlist->buddy_icon_window = 1;

	return userlist;
}

void
nateon_userlist_destroy(NateonUserList *userlist)
{
	GList *l;

	for (l = userlist->users; l != NULL; l = l->next)
	{
		nateon_user_destroy(l->data);
	}

	g_list_free(userlist->users);

	for (l = userlist->groups; l != NULL; l = l->next)
	{
		nateon_group_destroy(l->data);
	}

	g_list_free(userlist->groups);

//	g_queue_free(userlist->buddy_icon_requests);

//	if (userlist->buddy_icon_request_timer)
//		purple_timeout_remove(userlist->buddy_icon_request_timer);

	g_free(userlist);
}

void
nateon_userlist_add_user(NateonUserList *userlist, NateonUser *user)
{
	userlist->users = g_list_append(userlist->users, user);
}

//void
//nateon_userlist_remove_user(NateonUserList *userlist, NateonUser *user)
//{
//	userlist->users = g_list_remove(userlist->users, user);
//}

NateonUser *nateon_userlist_find_user_with_id(NateonUserList *userlist, const char *id)
{
	GList *l;

	g_return_val_if_fail(userlist != NULL, NULL);
	g_return_val_if_fail(id != NULL, NULL);

	for (l = userlist->users; l != NULL; l = l->next)
	{
		NateonUser *user = (NateonUser *)l->data;

		g_return_val_if_fail(user->id != NULL, NULL);

		if (!strcmp(id, user->id))
			return user;
	}

	return NULL;
}

NateonUser *nateon_userlist_find_user_with_name(NateonUserList *userlist, const char *account_name)
{
	GList *l;

	g_return_val_if_fail(userlist != NULL, NULL);
	g_return_val_if_fail(account_name != NULL, NULL);

	for (l = userlist->users; l != NULL; l = l->next)
	{
		NateonUser *user = (NateonUser *)l->data;

		g_return_val_if_fail(user->account_name != NULL, NULL);

		if (!strcmp(account_name, user->account_name))
			return user;
	}

	return NULL;
}

void
nateon_userlist_add_group(NateonUserList *userlist, NateonGroup *group)
{
	userlist->groups = g_list_append(userlist->groups, group);
}

//void
//nateon_userlist_remove_group(NateonUserList *userlist, NateonGroup *group)
//{
//	userlist->groups = g_list_remove(userlist->groups, group);
//}

NateonGroup *
nateon_userlist_find_group_with_id(NateonUserList *userlist, int id)
{
	GList *l;

	g_return_val_if_fail(userlist != NULL, NULL);
	g_return_val_if_fail(id       >= 0,    NULL);

	for (l = userlist->groups; l != NULL; l = l->next)
	{
		NateonGroup *group = l->data;

		if (group->id == id)
			return group;
	}

	return NULL;
}

NateonGroup *
nateon_userlist_find_group_with_name(NateonUserList *userlist, const char *name)
{
	GList *l;

	g_return_val_if_fail(userlist != NULL, NULL);
	g_return_val_if_fail(name     != NULL, NULL);

	for (l = userlist->groups; l != NULL; l = l->next)
	{
		NateonGroup *group = l->data;

		if ((group->name != NULL) && !g_ascii_strcasecmp(name, group->name))
			return group;
	}

	return NULL;
}

int
nateon_userlist_find_group_id(NateonUserList *userlist, const char *group_name)
{
	NateonGroup *group;

	group = nateon_userlist_find_group_with_name(userlist, group_name);

	if (group != NULL)
		return nateon_group_get_id(group);
	else
		return -1;
}

const char *
nateon_userlist_find_group_name(NateonUserList *userlist, int group_id)
{
	NateonGroup *group;

	group = nateon_userlist_find_group_with_id(userlist, group_id);

	if (group != NULL)
		return nateon_group_get_name(group);
	else
		return NULL;
}

//void
//nateon_userlist_rename_group_id(NateonUserList *userlist, int group_id,
//							 const char *new_name)
//{
//	NateonGroup *group;
//
//	group = nateon_userlist_find_group_with_id(userlist, group_id);
//
//	if (group != NULL)
//		nateon_group_set_name(group, new_name);
//}
//
//void
//nateon_userlist_remove_group_id(NateonUserList *userlist, int group_id)
//{
//	NateonGroup *group;
//
//	group = nateon_userlist_find_group_with_id(userlist, group_id);
//
//	if (group != NULL)
//	{
//		nateon_userlist_remove_group(userlist, group);
//		nateon_group_destroy(group);
//	}
//}

void
nateon_userlist_rem_buddy(NateonUserList *userlist,
					   const char *who, int list_id, const char *group_name)
{
	NateonUser *user;
	int group_id;
	const char *list;

	user = nateon_userlist_find_user_with_name(userlist, who);
	group_id = -1;

	if (group_name != NULL)
	{
		group_id = nateon_userlist_find_group_id(userlist, group_name);

		if (group_id < 0)
		{
			/* Whoa, there is no such group. */
			purple_debug_error("nateon", "Group doesn't exist: %s\n", group_name);
			return;
		}
	}

	/* First we're going to check if not there. */
	if (!(user_is_there(user, list_id, group_id)))
	{
		list = lists[list_id];
		purple_debug_error("nateon", "User '%s' is not there: %s\n",
						 who, list);
		return;
	}

	/* Then request the rem to the server. */
	list = lists[list_id];

	nateon_notification_rem_buddy(userlist->session->notification, list, who, group_id, user->id);
}

void
nateon_userlist_add_buddy(NateonUserList *userlist,
					   const char *who, int list_id,
					   const char *group_name)
{
	NateonUser *user;
	int group_id;
//	const char *list;
//	const char *store_name;

	group_id = -1;

	if (!purple_email_is_valid(who))
	{
		/* only notify the user about problems adding to the friends list
		 * maybe we should do something else for other lists, but it probably
		 * won't cause too many problems if we just ignore it */
		if (list_id == NATEON_LIST_FL)
		{
			char *str = g_strdup_printf(_("Unable to add \"%s\"."), who);
			purple_notify_error(NULL, NULL, str,
							  _("The screen name specified is invalid."));
			g_free(str);
		}

		return;
	}

	if (group_name != NULL)
	{
		group_id = nateon_userlist_find_group_id(userlist, group_name);

		if (group_id < 0)
		{
			/* Whoa, we must add that group first. */
			nateon_request_add_group(userlist, who, NULL, group_name);
			return;
		}
	}

	user = nateon_userlist_find_user_with_name(userlist, who);

//	/* First we're going to check if it's already there. */
//	if (user_is_there(user, list_id, group_id))
//	{
//		list = lists[list_id];
//		purple_debug_error("nateon", "User '%s' is already there: %s\n", who, list);
//		return;
//	}

//	store_name = (user != NULL) ? get_store_name(user) : who;
//
//	/* Then request the add to the server. */
//	list = lists[list_id];
//
//	nateon_notification_add_buddy(userlist->session->notification, list, who,
//							   store_name, group_id);
}

void
nateon_userlist_move_buddy(NateonUserList *userlist, const char *who,
						const char *old_group_name, const char *new_group_name)
{
	int new_group_id;

	new_group_id = nateon_userlist_find_group_id(userlist, new_group_name);

	if (new_group_id < 0)
	{
		nateon_request_add_group(userlist, who, old_group_name, new_group_name);
		return;
	}

//	nateon_userlist_add_buddy(userlist, who, NATEON_LIST_FL, new_group_name);
//	nateon_userlist_rem_buddy(userlist, who, NATEON_LIST_FL, old_group_name);
}
