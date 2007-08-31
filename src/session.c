/**
 * @file session.c NATEON session functions
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
#include "session.h"
#include "notification.h"

#include "dialog.h"

NateonSession *
nateon_session_new(PurpleAccount *account)
{
	NateonSession *session;

	g_return_val_if_fail(account != NULL, NULL);

	session = g_new0(NateonSession, 1);

	session->account = account;
	session->notification = nateon_notification_new(session);
	session->userlist = nateon_userlist_new(session);

	session->user = nateon_user_new(session->userlist,
		 purple_account_get_username(account), "", "");

	session->protocol_ver = NATEON_MAJOR_VERSION + (gfloat)NATEON_MINOR_VERSION / 1000;
	session->conv_seq = 1;

	return session;
}

void
nateon_session_destroy(NateonSession *session)
{
	g_return_if_fail(session != NULL);

	session->destroying = TRUE;

	if (session->connected)
		nateon_session_disconnect(session);

	if (session->notification != NULL)
		nateon_notification_destroy(session->notification);

	while (session->switches != NULL)
		nateon_switchboard_destroy(session->switches->data);

//	while (session->slplinks != NULL)
//		nateon_slplink_destroy(session->slplinks->data);

	nateon_userlist_destroy(session->userlist);

//	if (session->passport_info.kv != NULL)
//		g_free(session->passport_info.kv);
//
//	if (session->passport_info.sid != NULL)
//		g_free(session->passport_info.sid);
//
//	if (session->passport_info.mspauth != NULL)
//		g_free(session->passport_info.mspauth);
//
//	if (session->passport_info.client_ip != NULL)
//		g_free(session->passport_info.client_ip);
//
//	if (session->passport_info.file != NULL)
//	{
//		g_unlink(session->passport_info.file);
//		g_free(session->passport_info.file);
//	}

	if (session->sync != NULL)
		nateon_sync_destroy(session->sync);

//	if (session->nexus != NULL)
//		nateon_nexus_destroy(session->nexus);

	if (session->user != NULL)
		nateon_user_destroy(session->user);

	g_free(session);
}

gboolean
nateon_session_connect(NateonSession *session, const char *host, int port)
{
	g_return_val_if_fail(session != NULL, FALSE);
	g_return_val_if_fail(!session->connected, TRUE);

	session->connected = TRUE;
//	session->http_method = http_method;

	if (session->notification == NULL)
	{
		purple_debug_error("nateon", "This shouldn't happen\n");
		g_return_val_if_reached(FALSE);
	}

	if (nateon_notification_connect(session->notification, host, port))
	{
		return TRUE;
	}

	return FALSE;
}

void
nateon_session_disconnect(NateonSession *session)
{
	g_return_if_fail(session != NULL);
	g_return_if_fail(session->connected);

	session->connected = FALSE;

//	while (session->switches != NULL)
//		nateon_switchboard_close(session->switches->data);

	if (session->notification != NULL)
		nateon_notification_close(session->notification);
}

/* TODO: This must go away when conversation is redesigned */
NateonSwitchBoard *
nateon_session_find_swboard(NateonSession *session, const char *username)
{
	GList *l;

	g_return_val_if_fail(session  != NULL, NULL);
	g_return_val_if_fail(username != NULL, NULL);

	for (l = session->switches; l != NULL; l = l->next)
	{
		NateonSwitchBoard *swboard;

		swboard = l->data;

		if ((swboard->im_user != NULL) && !strcmp(username, swboard->im_user))
			return swboard;
	}

	return NULL;
}

NateonSwitchBoard *
nateon_session_find_swboard_with_conv(NateonSession *session, PurpleConversation *conv)
{
	GList *l;

	g_return_val_if_fail(session  != NULL, NULL);
	g_return_val_if_fail(conv != NULL, NULL);

	for (l = session->switches; l != NULL; l = l->next)
	{
		NateonSwitchBoard *swboard;

		swboard = l->data;

		if (swboard->conv == conv)
			return swboard;
	}

	return NULL;
}

//NateonSwitchBoard *
//nateon_session_find_swboard_with_id(const NateonSession *session, int chat_id)
//{
//	GList *l;
//
//	g_return_val_if_fail(session != NULL, NULL);
//	g_return_val_if_fail(chat_id >= 0,    NULL);
//
//	for (l = session->switches; l != NULL; l = l->next)
//	{
//		NateonSwitchBoard *swboard;
//
//		swboard = l->data;
//
//		if (swboard->chat_id == chat_id)
//			return swboard;
//	}
//
//	return NULL;
//}

NateonSwitchBoard *
nateon_session_get_swboard(NateonSession *session, const char *username,
						NateonSBFlag flag)
{
	NateonSwitchBoard *swboard;

	g_return_val_if_fail(session != NULL, NULL);

	swboard = nateon_session_find_swboard(session, username);

	if (swboard == NULL)
	{
		swboard = nateon_switchboard_new(session);
		swboard->im_user = g_strdup(username);
		nateon_switchboard_request(swboard);
		//nateon_switchboard_request_add_user(swboard, username);
	}

	swboard->flag |= flag;

	return swboard;
}

static void
nateon_session_sync_users(NateonSession *session)
{
	PurpleBlistNode *gnode, *cnode, *bnode;
	PurpleConnection *gc = purple_account_get_connection(session->account);

	g_return_if_fail(gc != NULL);

	/* The core used to use nateon_add_buddy to add all buddies before
	 * being logged in. This no longer happens, so we manually iterate
	 * over the whole buddy list to identify sync issues. */

	for (gnode = purple_blist_get_root(); gnode; gnode = gnode->next) {
		PurpleGroup *group = (PurpleGroup *)gnode;
		const char *group_name = group->name;
		if(!PURPLE_BLIST_NODE_IS_GROUP(gnode))
			continue;
		for(cnode = gnode->child; cnode; cnode = cnode->next) {
			if(!PURPLE_BLIST_NODE_IS_CONTACT(cnode))
				continue;
			for(bnode = cnode->child; bnode; bnode = bnode->next) {
				PurpleBuddy *b;
				if(!PURPLE_BLIST_NODE_IS_BUDDY(bnode))
					continue;
				b = (PurpleBuddy *)bnode;
				if(purple_buddy_get_account(b) == purple_connection_get_account(gc)) {
					NateonUser *remote_user;
					gboolean found = FALSE;

					remote_user = nateon_userlist_find_user_with_name(session->userlist, purple_buddy_get_name(b));

					if ((remote_user != NULL) && (remote_user->list_op & NATEON_LIST_FL_OP))
					{
						int group_id;
						GList *l;

						group_id = nateon_userlist_find_group_id(remote_user->userlist, group_name);

						for (l = remote_user->group_ids; l != NULL; l = l->next)
						{

							if (group_id == GPOINTER_TO_INT(l->data))
							{
								found = TRUE;
								break;
							}
						}

					}

					if (!found)
					{
						purple_debug_info("nateon", "%s: somthing wrong?\n", __FUNCTION__);
						/* The user was not on the server list or not in that group
						 * on the server list */
						nateon_show_sync_issue(session, purple_buddy_get_name(b), group_name);
						break;
					}
				}
			}
		}
	}
}

void
nateon_session_set_error(NateonSession *session, NateonErrorType error,
					  const char *info)
{
	PurpleConnection *gc;
	char *msg;

	gc = purple_account_get_connection(session->account);

	switch (error)
	{
//		case NATEON_ERROR_SERVCONN:
//			msg = g_strdup(info);
//			break;
//		case NATEON_ERROR_UNSUPPORTED_PROTOCOL:
//			msg = g_strdup(_("Our protocol is not supported by the "
//							 "server."));
//			break;
//		case NATEON_ERROR_HTTP_MALFORMED:
//			msg = g_strdup(_("Error parsing HTTP."));
//			break;
		case NATEON_ERROR_SIGN_OTHER:
			gc->wants_to_die = TRUE;
			msg = g_strdup(_("You have signed on from another location."));
			break;
//		case NATEON_ERROR_SERV_UNAVAILABLE:
//			msg = g_strdup(_("The NATEON servers are temporarily "
//							 "unavailable. Please wait and try "
//							 "again."));
//			break;
//		case NATEON_ERROR_SERV_DOWN:
//			msg = g_strdup(_("The NATEON servers are going down "
//							 "temporarily."));
//			break;
		case NATEON_ERROR_AUTH:
			msg = g_strdup_printf(_("Unable to authenticate: %s"),
								  (info == NULL ) ?
								  _("Unknown error") : info);
			break;
//		case NATEON_ERROR_BAD_BLIST:
//			msg = g_strdup(_("Your NATEON buddy list is temporarily "
//							 "unavailable. Please wait and try "
//							 "again."));
//			break;
		default:
			msg = g_strdup(_("Unknown error."));
			break;
	}

	nateon_session_disconnect(session);

	purple_connection_error(gc, msg);

	g_free(msg);
}

static const char *
get_login_step_text(NateonSession *session)
{
	const char *steps_text[] = {
		_("Connecting"),
		_("Handshaking"),
		_("Transferring"),
		_("Handshaking"),
		_("Starting authentication"),
		_("Getting cookie"),
		_("Authenticating"),
		_("Sending cookie"),
		_("Retrieving buddy list")
	};

	return steps_text[session->login_step];
}

void
nateon_session_set_login_step(NateonSession *session, NateonLoginStep step)
{
	PurpleConnection *gc;

	/* Prevent the connection progress going backwards, eg. if we get
	 * transferred several times during login */
	if (session->login_step > step)
		return;

	/* If we're already logged in, we're probably here because of a
	 * mid-session XFR from the notification server, so we don't want to
	 * popup the connection progress dialog */
	if (session->logged_in)
		return;

	gc = session->account->gc;

	session->login_step = step;

	purple_connection_update_progress(gc, get_login_step_text(session), step,
		NATEON_LOGIN_STEPS);
}

void
nateon_session_finish_login(NateonSession *session)
{
	PurpleAccount *account;
	PurpleConnection *gc;
	PurpleStoredImage *img;

	if (session->logged_in)
		return;

	account = session->account;
	gc = purple_account_get_connection(account);

	img = purple_buddy_icons_find_account_icon(session->account);
	nateon_user_set_buddy_icon(session->user, img);
	purple_imgstore_unref(img);

	session->logged_in = TRUE;

	nateon_change_status(session);

	purple_connection_set_state(gc, PURPLE_CONNECTED);

	/* Sync users */
	nateon_session_sync_users(session);
}
