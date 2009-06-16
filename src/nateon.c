/**
 * @file nateon.c The NateOn protocol plugin
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
#define PHOTO_SUPPORT 1

#include <glib.h>

#include "nateon.h"
#include "accountopt.h"
#include "msg.h"
//#include "page.h"
#include "memo.h"
#include "pluginpref.h"
#include "prefs.h"
#include "session.h"
#include "state.h"
//#include "utils.h"
#include "cmds.h"
#include "core.h"
#include "prpl.h"
#include "util.h"
#include "nateon-utils.h"
#include "version.h"

#include "switchboard.h"
#include "notification.h"
#include "sync.h"
#include "xfer.h"
//#include "slplink.h"

#if PHOTO_SUPPORT
#include "imgstore.h"
#endif

typedef struct
{
	PurpleConnection *gc;
	const char *account;

} NateonSendData;

//typedef struct
//{
//	PurpleConnection *gc;
//	char *name;
//
//} NateonGetInfoData;
//
//typedef struct
//{
//	NateonGetInfoData *info_data;
//	char *stripped;
//	char *url_buffer;
//	GString *s;
//	char *photo_url_text;
//	char *tooltip_text;
//	const char *title;
//
//} NateonGetInfoStepTwoData;

static const char *nateon_normalize(const PurpleAccount *account, const char *str)
{
	static char buf[BUF_LEN];
	char *tmp;

	g_return_val_if_fail(str != NULL, NULL);

	g_snprintf(buf, sizeof(buf), "%s%s", str,
			(strchr(str, '@') ? "" : "@nate.com"));

	tmp = g_utf8_strdown(buf, -1);
	strncpy(buf, tmp, sizeof(buf));
	g_free(tmp);

	return buf;
}

//static PurpleCmdRet
//nateon_cmd_nudge(PurpleConversation *conv, const gchar *cmd, gchar **args, gchar **error, void *data)
//{
//	PurpleAccount *account = purple_conversation_get_account(conv);
//	PurpleConnection *gc = purple_account_get_connection(account);
//	NateonMessage *msg;
//	NateonSession *session;
//	NateonSwitchBoard *swboard;
//
//	msg = nateon_message_new_nudge();
//	session = gc->proto_data;
//	swboard = nateon_session_get_swboard(session, purple_conversation_get_name(conv), NATEON_SB_FLAG_IM);
//
//	if (session == NULL || swboard == NULL)
//		return PURPLE_CMD_RET_FAILED;
//
//	nateon_switchboard_send_msg(swboard, msg, TRUE);
//
//	purple_conversation_write(conv, NULL, _("You have just sent a Nudge!"), PURPLE_MESSAGE_SYSTEM, time(NULL));
//
//	return PURPLE_CMD_RET_OK;
//}

static void nateon_act_id(PurpleConnection *gc, const char *entry)
{
	NateonCmdProc *cmdproc;
	NateonSession *session;
	PurpleAccount *account;
	const char *alias;

	session = gc->proto_data;
	cmdproc = session->notification->cmdproc;
	account = purple_connection_get_account(gc);

	if(entry && strlen(entry))
		alias = purple_strreplace(entry, " ", "%20");
	else
		alias = "";

	if (strlen(alias) > BUDDY_ALIAS_MAXLEN)
	{
		purple_notify_error(gc, NULL, _("Your new NateOn friendly name is too long."), NULL);
		return;
	}

	nateon_cmdproc_send(cmdproc, "CNIK", "%s", alias);
}

static void nateon_act_view_buddies_by(PurpleAccount *account, int choice)
{
	PurpleBlistNode *gnode, *cnode, *bnode;
	PurpleConnection *gc;

	gc = purple_account_get_connection(account);

	purple_account_set_int(account, "view_buddies_by", choice);

	for (gnode = purple_blist_get_root(); gnode; gnode = gnode->next) {
		//PurpleGroup *group = (PurpleGroup *)gnode;
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
				if(purple_buddy_get_account(b) == account) {
					nateon_user_set_buddy_alias(gc->proto_data, b->proto_data);
				}
			}
		}
	}
}

//static void
//nateon_set_prp(PurpleConnection *gc, const char *type, const char *entry)
//{
//	NateonCmdProc *cmdproc;
//	NateonSession *session;
//
//	session = gc->proto_data;
//	cmdproc = session->notification->cmdproc;
//
//	if (entry == NULL || *entry == '\0')
//	{
//		nateon_cmdproc_send(cmdproc, "PRP", "%s", type);
//	}
//	else
//	{
//		nateon_cmdproc_send(cmdproc, "PRP", "%s %s", type,
//						 purple_url_encode(entry));
//	}
//}
//
//static void
//nateon_set_home_phone_cb(PurpleConnection *gc, const char *entry)
//{
//	nateon_set_prp(gc, "PHH", entry);
//}
//
//static void
//nateon_set_work_phone_cb(PurpleConnection *gc, const char *entry)
//{
//	nateon_set_prp(gc, "PHW", entry);
//}
//
//static void
//nateon_set_mobile_phone_cb(PurpleConnection *gc, const char *entry)
//{
//	nateon_set_prp(gc, "PHM", entry);
//}
//
//static void
//enable_nateon_pages_cb(PurpleConnection *gc)
//{
//	nateon_set_prp(gc, "MOB", "Y");
//}
//
//static void
//disable_nateon_pages_cb(PurpleConnection *gc)
//{
//	nateon_set_prp(gc, "MOB", "N");
//}
//
//static void
//send_to_mobile(PurpleConnection *gc, const char *who, const char *entry)
//{
//	NateonTransaction *trans;
//	NateonSession *session;
//	NateonCmdProc *cmdproc;
//	NateonPage *page;
//	char *payload;
//	size_t payload_len;
//
//	session = gc->proto_data;
//	cmdproc = session->notification->cmdproc;
//
//	page = nateon_page_new();
//	nateon_page_set_body(page, entry);
//
//	payload = nateon_page_gen_payload(page, &payload_len);
//
//	trans = nateon_transaction_new(cmdproc, "PGD", "%s 1 %d", who, payload_len);
//
//	nateon_transaction_set_payload(trans, payload, payload_len);
//
//	nateon_page_destroy(page);
//
//	nateon_cmdproc_send_trans(cmdproc, trans);
//}
//
//static void
//send_to_mobile_cb(NateonMobileData *data, const char *entry)
//{
//	send_to_mobile(data->gc, data->passport, entry);
//	g_free(data);
//}

//static void
//close_mobile_page_cb(NateonMobileData *data, const char *entry)
//{
//	g_free(data);
//}

	static void
send_memo(PurpleConnection *gc, const char *who, const char *entry)
{
	PurpleAccount *account;
	NateonTransaction *trans;
	NateonSession *session;
	NateonCmdProc *cmdproc;
	NateonMemo *memo;
	char *payload;
	size_t payload_len;

	account = purple_connection_get_account(gc);
	session = gc->proto_data;
	cmdproc = session->notification->cmdproc;

	memo = nateon_memo_new(purple_account_get_username(account), who);
	nateon_memo_set_body(memo, entry);

	payload = nateon_memo_gen_payload(memo, &payload_len);

	trans = nateon_transaction_new(cmdproc, "CMSG", "N %d", payload_len);

	nateon_transaction_set_payload(trans, payload, payload_len);

	nateon_memo_destroy(memo);

	nateon_cmdproc_send_trans(cmdproc, trans);
}

static void
send_memo_cb(NateonSendData *data, const char *entry)
{
	send_memo(data->gc, data->account, entry);
	g_free(data);
}

static void
close_memo_cb(NateonSendData *data, const char *entry)
{
	g_free(data);
}

/* -- */

	static void
nateon_show_set_friendly_name(PurplePluginAction *action)
{
	PurpleConnection *gc;

	gc = (PurpleConnection *) action->context;

	purple_request_input(gc, NULL, _("Set your friendly name."),
			_("This is the name that other NATEON buddies will "
				"see you as."),
			purple_connection_get_display_name(gc), FALSE, FALSE, NULL,
			_("OK"), G_CALLBACK(nateon_act_id),
			_("Cancel"), NULL,
			purple_connection_get_account(gc), NULL, NULL,
			gc);
}

	static void
nateon_show_view_buddies_by(PurplePluginAction *action)
{
	PurpleConnection *gc;
	PurpleAccount	 *account;

	gc = (PurpleConnection *) action->context;
	account = purple_connection_get_account(gc);

	purple_request_choice(gc, _("View Buddies By."), 
			_("View Buddies By."),
			NULL, purple_account_get_int(account, "view_buddies_by", NATEON_VIEW_BUDDIES_BY_SCREEN_NAME),
			_("OK"), G_CALLBACK(nateon_act_view_buddies_by),
			_("Cancel"), NULL,
			account, NULL, NULL,
			account,
			_("Name"), NATEON_VIEW_BUDDIES_BY_NAME,
			_("Screen Name"), NATEON_VIEW_BUDDIES_BY_SCREEN_NAME,
			_("Name and ID"), NATEON_VIEW_BUDDIES_BY_NAME_AND_ID,
			_("Name and Screen Name"), NATEON_VIEW_BUDDIES_BY_NAME_AND_SCREEN_NAME,
			NULL);
}

static void nateon_show_send_sms(PurplePluginAction *action)
{
	PurpleConnection *gc;
	PurpleAccount *account;
	NateonSession *session;
	const char *username;
	char *uri;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	gc = (PurpleConnection *) action->context;
	account = purple_connection_get_account(gc);
	session = gc->proto_data;
	username = purple_account_get_username(account);

	uri = g_strdup_printf("http://br.nate.com/index.php?code=G047&param=TICKET%%3D%s%%26ID%%3d%s%%26mobile%%3d", session->ticket, username);
	purple_notify_uri(gc, uri);
	g_free(uri);
}

//static void
//nateon_show_set_home_phone(PurplePluginAction *action)
//{
//	PurpleConnection *gc;
//	NateonSession *session;
//
//	gc = (PurpleConnection *) action->context;
//	session = gc->proto_data;
//
//	purple_request_input(gc, NULL, _("Set your home phone number."), NULL,
//					   nateon_user_get_home_phone(session->user), FALSE, FALSE, NULL,
//					   _("OK"), G_CALLBACK(nateon_set_home_phone_cb),
//					   _("Cancel"), NULL, gc);
//}
//
//static void
//nateon_show_set_work_phone(PurplePluginAction *action)
//{
//	PurpleConnection *gc;
//	NateonSession *session;
//
//	gc = (PurpleConnection *) action->context;
//	session = gc->proto_data;
//
//	purple_request_input(gc, NULL, _("Set your work phone number."), NULL,
//					   nateon_user_get_work_phone(session->user), FALSE, FALSE, NULL,
//					   _("OK"), G_CALLBACK(nateon_set_work_phone_cb),
//					   _("Cancel"), NULL, gc);
//}
//
//static void
//nateon_show_set_mobile_phone(PurplePluginAction *action)
//{
//	PurpleConnection *gc;
//	NateonSession *session;
//
//	gc = (PurpleConnection *) action->context;
//	session = gc->proto_data;
//
//	purple_request_input(gc, NULL, _("Set your mobile phone number."), NULL,
//					   nateon_user_get_mobile_phone(session->user), FALSE, FALSE, NULL,
//					   _("OK"), G_CALLBACK(nateon_set_mobile_phone_cb),
//					   _("Cancel"), NULL, gc);
//}
//
//static void
//nateon_show_set_mobile_pages(PurplePluginAction *action)
//{
//	PurpleConnection *gc;
//
//	gc = (PurpleConnection *) action->context;
//
//	purple_request_action(gc, NULL, _("Allow NATEON Mobile pages?"),
//						_("Do you want to allow or disallow people on "
//						  "your buddy list to send you NATEON Mobile pages "
//						  "to your cell phone or other mobile device?"),
//						-1, gc, 3,
//						_("Allow"), G_CALLBACK(enable_nateon_pages_cb),
//						_("Disallow"), G_CALLBACK(disable_nateon_pages_cb),
//						_("Cancel"), NULL);
//}
//
//static void
//nateon_show_hotmail_inbox(PurplePluginAction *action)
//{
//	PurpleConnection *gc;
//	NateonSession *session;
//
//	gc = (PurpleConnection *) action->context;
//	session = gc->proto_data;
//
//	if (session->passport_info.file == NULL)
//	{
//		purple_notify_error(gc, NULL,
//						  _("This Hotmail account may not be active."), NULL);
//		return;
//	}
//
//	purple_notify_uri(gc, session->passport_info.file);
//}
//
//static void
//show_send_to_mobile_cb(PurpleBlistNode *node, gpointer ignored)
//{
//	PurpleBuddy *buddy;
//	PurpleConnection *gc;
//	NateonSession *session;
//	NateonMobileData *data;
//
//	g_return_if_fail(PURPLE_BLIST_NODE_IS_BUDDY(node));
//
//	buddy = (PurpleBuddy *) node;
//	gc = purple_account_get_connection(buddy->account);
//
//	session = gc->proto_data;
//
//	data = g_new0(NateonMobileData, 1);
//	data->gc = gc;
//	data->passport = buddy->name;
//
//	purple_request_input(gc, NULL, _("Send a mobile message."), NULL,
//					   NULL, TRUE, FALSE, NULL,
//					   _("Page"), G_CALLBACK(send_to_mobile_cb),
//					   _("Close"), G_CALLBACK(close_mobile_page_cb),
//					   data);
//}

static void
show_send_memo_cb(PurpleBlistNode *node, gpointer ignored)
{
	PurpleBuddy *buddy;
	PurpleConnection *gc;
	NateonSession *session;
	NateonSendData *data;

	g_return_if_fail(PURPLE_BLIST_NODE_IS_BUDDY(node));

	buddy = (PurpleBuddy *) node;
	gc = purple_account_get_connection(buddy->account);

	session = gc->proto_data;

	data = g_new0(NateonSendData, 1);
	data->gc = gc;
	data->account = buddy->name;

	purple_request_input(gc, NULL, buddy->name, NULL,
			NULL, TRUE, FALSE, NULL,
			_("_Send"), G_CALLBACK(send_memo_cb),
			_("Close"), G_CALLBACK(close_memo_cb),
			purple_connection_get_account(gc), NULL, NULL,
			data);
}

static void
show_send_sms_cb(PurpleBlistNode *node, gpointer ignored)
{
	PurpleBuddy *buddy;
	PurpleConnection *gc;
	NateonSession *session;
	const char *username;
	char *uri;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	g_return_if_fail(PURPLE_BLIST_NODE_IS_BUDDY(node));

	buddy = (PurpleBuddy *) node;
	gc = purple_account_get_connection(buddy->account);
	session = gc->proto_data;
	username = purple_account_get_username(buddy->account);

	uri = g_strdup_printf("http://br.nate.com/index.php?code=G047&param=TICKET%%3D%s%%26ID%%3d%s%%26mobile%%3d", session->ticket, username);
	purple_notify_uri(gc, uri);
	g_free(uri);
}

//static void
//initiate_chat_cb(PurpleBlistNode *node, gpointer data)
//{
//	PurpleBuddy *buddy;
//	PurpleConnection *gc;
//
//	NateonSession *session;
//	NateonSwitchBoard *swboard;
//
//	g_return_if_fail(PURPLE_BLIST_NODE_IS_BUDDY(node));
//
//	buddy = (PurpleBuddy *) node;
//	gc = purple_account_get_connection(buddy->account);
//
//	session = gc->proto_data;
//
//	swboard = nateon_switchboard_new(session);
//	nateon_switchboard_request(swboard);
//	nateon_switchboard_request_add_user(swboard, buddy->name);
//
//	/* TODO: This might move somewhere else, after USR might be */
//	swboard->chat_id = session->conv_seq++;
//	swboard->conv = serv_got_joined_chat(gc, swboard->chat_id, "NATEON Chat");
//	swboard->flag = NATEON_SB_FLAG_IM;
//
//	purple_conv_chat_add_user(PURPLE_CONV_CHAT(swboard->conv),
//							purple_account_get_username(buddy->account), NULL, PURPLE_CBFLAGS_NONE, TRUE);
//}

//static void
//t_nateon_xfer_init(PurpleXfer *xfer)
//{
////	NateonSlpLink *slplink;
//	const char *filename;
//	FILE *fp;
//
//	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
//
//	filename = purple_xfer_get_local_filename(xfer);
//	purple_debug_info("nateon", "[%s] filename(%s)\n", __FUNCTION__, filename);
//
////	slplink = xfer->data;
//
//	if ((fp = g_fopen(filename, "rb")) == NULL)
//	{
//		purple_debug_info("nateon", "[%s] xfer_error\n", __FUNCTION__, filename);
////		PurpleAccount *account;
////		PurpleConnection *gc;
////		const char *who;
////		char *msg;
////
////		account = slplink->session->account;
////		gc = purple_account_get_connection(account);
////		who = slplink->remote_user;
////
////		msg = g_strdup_printf(_("Error reading %s: \n%s.\n"),
////							  filename, strerror(errno));
////		purple_xfer_error(purple_xfer_get_type(xfer), account, xfer->who, msg);
////		purple_xfer_cancel_local(xfer);
////		g_free(msg);
////
//		return;
//	}
//	fclose(fp);
//
////	nateon_slplink_request_ft(slplink, xfer);
//}

//static PurpleXfer*
//nateon_new_xfer(PurpleConnection *gc, const char *who)
//{
//	NateonSession *session;
//	NateonSlpLink *slplink;
//	PurpleXfer *xfer;
//
//	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
//
//	session = gc->proto_data;
//
//	xfer = purple_xfer_new(gc->account, PURPLE_XFER_SEND, who);
//
////	slplink = nateon_session_get_slplink(session, who);
//
//	xfer->data = slplink;
//
//	purple_xfer_set_init_fnc(xfer, t_nateon_xfer_init);
//
//	return xfer;
//}

	static void
nateon_send_file(PurpleConnection *gc, const char *who, const char *file)
{
	nateon_xfer_send_file(gc->proto_data, who, file);
}

	static gboolean
nateon_can_receive_file(PurpleConnection *gc, const char *who)
{
	NateonSession *session;
	PurpleAccount *account;
	char *normal;
	GList *l;
	gboolean ret;

	account = purple_connection_get_account(gc);

	normal = g_strdup(nateon_normalize(account, purple_account_get_username(account)));

	ret = strcmp(normal, nateon_normalize(account, who));

	g_free(normal);

	if (ret == 0)
	{
		/* Can't send to oneself */
		return FALSE;
	}

	/* Can't send more than one file at same time */
	session = gc->proto_data;
	ret = TRUE;
	for (l = session->xfers; l != NULL; l = l->next)
	{
		NateonXfer *xfer;
		xfer = l->data;
		if (purple_xfer_get_type(xfer->prpl_xfer) == PURPLE_XFER_SEND &&
				!strcmp(xfer->who, who))
		{
			ret = FALSE;
			break;
		}
	}

	return ret;
}

/**************************************************************************
 * Protocol Plugin ops
 **************************************************************************/

static const char* nateon_list_icon(PurpleAccount *a, PurpleBuddy *b)
{
	return "nateon";
}

static char* nateon_status_text(PurpleBuddy *buddy)
{
	PurplePresence *presence;
	PurpleStatus *status;

	presence = purple_buddy_get_presence(buddy);
	status = purple_presence_get_active_status(presence);

	if (!purple_presence_is_available(presence) && !purple_presence_is_idle(presence))
	{
		return g_strdup(purple_status_get_name(status));
	}

	return NULL;
}

	static void
nateon_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *user_info, gboolean full)
{
	NateonUser *user;
	PurplePresence *presence = purple_buddy_get_presence(buddy);
	PurpleStatus *status = purple_presence_get_active_status(presence);

	user = buddy->proto_data;

	if (user)
	{
		purple_notify_user_info_add_pair(user_info, _("Name"), user->friendly_name);
	}

	if (purple_presence_is_online(presence))
	{
		purple_notify_user_info_add_pair(user_info, _("Status"), purple_presence_is_idle(presence) ? _("Idle") : purple_status_get_name(status));
	}

	//	if (full && user)
	//	{
	//		purple_notify_user_info_add_pair(user_info, _("Has you"), ((user->list_op & (1 << NATEON_LIST_RL)) ? _("Yes") : _("No")));
	//
	//	}
	//
	//	/* XXX: This is being shown in non-full tooltips because the
	//	 * XXX: blocked icon overlay isn't always accurate for NATEON.
	//	 * XXX: This can die as soon as purple_privacy_check() knows that
	//	 * XXX: this prpl always honors both the allow and deny lists. */
	//	if (user)
	//	{
	//		purple_notify_user_info_add_pair(user_info, _("Blocked"), ((user->list_op & (1 << NATEON_LIST_BL)) ? _("Yes") : _("No")));
	//	}
}

	static GList *
nateon_status_types(PurpleAccount *account)
{
	PurpleStatusType *status;
	GList *types = NULL;

	status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE,
			"O", NULL, FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_AWAY,
			"A", NULL, FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_UNAVAILABLE,
			"B", _("Busy"), FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_UNAVAILABLE,
			"P", _("On The Phone"), FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_AWAY,
			"M", _("In meeting"), FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_INVISIBLE,
			"X", NULL, FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE,
			NULL, NULL, FALSE, TRUE, FALSE);
	types = g_list_append(types, status);

	return types;
}

	static GList *
nateon_actions(PurplePlugin *plugin, gpointer context)
{
	//	PurpleConnection *gc = (PurpleConnection *)context;
	//	PurpleAccount *account;
	//	const char *user;

	GList *m = NULL;
	PurplePluginAction *act;

	act = purple_plugin_action_new(_("Set Friendly Name..."), nateon_show_set_friendly_name);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);

	act = purple_plugin_action_new(_("View Buddies By..."), nateon_show_view_buddies_by);
	m = g_list_append(m, act);
	m = g_list_append(m, NULL);

	act = purple_plugin_action_new(_("Send SMS message..."), nateon_show_send_sms);
	m = g_list_append(m, act);

	//	act = purple_plugin_action_new(_("Set Home Phone Number..."),
	//								 nateon_show_set_home_phone);
	//	m = g_list_append(m, act);
	//
	//	act = purple_plugin_action_new(_("Set Work Phone Number..."),
	//			nateon_show_set_work_phone);
	//	m = g_list_append(m, act);
	//
	//	act = purple_plugin_action_new(_("Set Mobile Phone Number..."),
	//			nateon_show_set_mobile_phone);
	//	m = g_list_append(m, act);
	//	m = g_list_append(m, NULL);
	//
	//#if 0
	//	act = purple_plugin_action_new(_("Enable/Disable Mobile Devices..."),
	//			nateon_show_set_mobile_support);
	//	m = g_list_append(m, act);
	//#endif
	//
	//	act = purple_plugin_action_new(_("Allow/Disallow Mobile Pages..."),
	//			nateon_show_set_mobile_pages);
	//	m = g_list_append(m, act);
	//
	//	account = purple_connection_get_account(gc);
	//	user = nateon_normalize(account, purple_account_get_username(account));
	//
	//	if (strstr(user, "@hotmail.com") != NULL)
	//	{
	//		m = g_list_append(m, NULL);
	//		act = purple_plugin_action_new(_("Open Hotmail Inbox"),
	//				nateon_show_hotmail_inbox);
	//		m = g_list_append(m, act);
	//	}

	return m;
}

	static GList *
nateon_buddy_menu(PurpleBuddy *buddy)
{
	NateonUser *user;

	GList *m = NULL;
	PurpleMenuAction *act;

	g_return_val_if_fail(buddy != NULL, NULL);

	user = buddy->proto_data;

	if (user != NULL)
	{
		PurpleBlistNode *gnode;
		GList *m_copy = NULL;
		GList *m_move = NULL;

		//		if (user->mobile)
		//		{
		//			act = purple_menu_action_new(_("Send to Mobile"),
		//			                           PURPLE_CALLBACK(show_send_to_mobile_cb),
		//			                           NULL, NULL);
		//			m = g_list_append(m, act);
		//		}
		//		
		act = purple_menu_action_new(_("Send memo"), PURPLE_CALLBACK(show_send_memo_cb), NULL, NULL);
		m = g_list_append(m, act);

		act = purple_menu_action_new(_("Send SMS message"), PURPLE_CALLBACK(show_send_sms_cb), NULL, NULL);
		m = g_list_append(m, act);

		/* Copy buddy */
		for (gnode = purple_blist_get_root(); gnode; gnode = gnode->next)
		{
			PurpleGroup *group = (PurpleGroup *)gnode;

			if(!PURPLE_BLIST_NODE_IS_GROUP(gnode))
				continue;

			act = purple_menu_action_new(group->name, NULL, NULL, NULL);
			m_copy = g_list_append(m_copy, act);
		}
		act = purple_menu_action_new(_("Copy buddy"), NULL, NULL, m_copy);
		m = g_list_append(m, act);
	}

	if (g_ascii_strcasecmp(buddy->name,
				purple_account_get_username(buddy->account)))
	{
		act = purple_menu_action_new(_("Initiate _Chat"), NULL, NULL, NULL);
		//		act = purple_menu_action_new(_("Initiate _Chat"),
		//		                           PURPLE_CALLBACK(initiate_chat_cb),
		//		                           NULL, NULL);
		m = g_list_append(m, act);
	}

	return m;
}

static GList *nateon_blist_node_menu(PurpleBlistNode *node)
{
	if(PURPLE_BLIST_NODE_IS_BUDDY(node))
	{
		return nateon_buddy_menu((PurpleBuddy *) node);
	}
	else
	{
		return NULL;
	}
}

	static void
nateon_login(PurpleAccount *account)
{
	PurpleConnection *gc;
	NateonSession *session;
	const char *username;
	const char *host;
	gboolean prs_method = FALSE;
	int port;

	gc = purple_account_get_connection(account);

	//	if (!purple_ssl_is_supported())
	//	{
	//		gc->wants_to_die = TRUE;
	//		purple_connection_error(gc,
	//			_("SSL support is needed for NATEON. Please install a supported "
	//			  "SSL library. See http://purple.sf.net/faq-ssl.php for more "
	//			  "information."));
	//
	//		return;
	//	}
	//
	prs_method = purple_account_get_bool(account, "prs_method", FALSE);

	host = purple_account_get_string(account, "server", NATEON_SERVER);
	port = purple_account_get_int(account, "port", NATEON_PORT);

	session = nateon_session_new(account);

	gc->proto_data = session;
	gc->flags |= PURPLE_CONNECTION_HTML | PURPLE_CONNECTION_FORMATTING_WBFO | PURPLE_CONNECTION_NO_BGCOLOR | PURPLE_CONNECTION_NO_FONTSIZE | PURPLE_CONNECTION_NO_URLDESC;

	nateon_session_set_login_step(session, NATEON_LOGIN_STEP_START);

	/* Hmm, I don't like this. */
	/* XXX shx: Me neither */
	username = nateon_normalize(account, purple_account_get_username(account));

	if (strcmp(username, purple_account_get_username(account)))
		purple_account_set_username(account, username);

	if (!nateon_session_connect(session, host, port, prs_method))
		purple_connection_error(gc, _("Failed to connect to server."));
}

	static void
nateon_close(PurpleConnection *gc)
{
	NateonSession *session;

	session = gc->proto_data;

	g_return_if_fail(session != NULL);

	nateon_session_destroy(session);

	gc->proto_data = NULL;
}

	static int
nateon_send_im(PurpleConnection *gc, const char *who, const char *message,
		PurpleMessageFlags flags)
{
	PurpleAccount *account;
	//	PurpleBuddy *buddy = purple_find_buddy(gc->account, who);
	NateonMessage *msg;
	//	char *msgformat;
	char *msgtext;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	account = purple_connection_get_account(gc);

	//	if (buddy)
	//	{
	//                PurplePresence *p = purple_buddy_get_presence(buddy);
	//                if (purple_presence_is_status_primitive_active(p, PURPLE_STATUS_MOBILE)) {
	//                        char *text = purple_markup_strip_html(message);
	//                        send_to_mobile(gc, who, text);
	//                        g_free(text);
	//                        return 1;
	//                }
	//        }

	msgtext = nateon_import_html(message);
	purple_debug_info("nateon", "message:%s\n", message);
	//	purple_debug_info("nateon", "format :%s\n", msgformat);
	purple_debug_info("nateon", "text   :%s\n", msgtext);


	//	if (strlen(msgtext) + strlen(msgformat) + strlen(VERSION) > 1564)
	//	{
	//		g_free(msgformat);
	//		g_free(msgtext);
	//
	//		return -E2BIG;
	//	}

	msg = nateon_message_new_plain(msgtext);
	//	nateon_message_set_attr(msg, "X-MMS-IM-Format", msgformat);

	//	g_free(msgformat);
	g_free(msgtext);

	if (g_ascii_strcasecmp(who, purple_account_get_username(account)))
	{
		NateonSession *session;
		NateonSwitchBoard *swboard;

		session = gc->proto_data;
		swboard = nateon_session_get_swboard(session, who, NATEON_SB_FLAG_IM);

		nateon_switchboard_send_msg(swboard, msg, TRUE);
	}
	//	else
	//	{
	//		char *body_str, *body_enc, *pre, *post;
	//		const char *format;
	//		/*
	//		 * In NATEON, you can't send messages to yourself, so
	//		 * we'll fake like we received it ;)
	//		 */
	//		body_str = nateon_message_to_string(msg);
	//		body_enc = g_markup_escape_text(body_str, -1);
	//		g_free(body_str);
	//
	//		format = nateon_message_get_attr(msg, "X-MMS-IM-Format");
	//		nateon_parse_format(format, &pre, &post);
	//		body_str = g_strdup_printf("%s%s%s", pre ? pre :  "",
	//								   body_enc ? body_enc : "", post ? post : "");
	//		g_free(body_enc);
	//		g_free(pre);
	//		g_free(post);
	//
	//		serv_got_typing_stopped(gc, who);
	//		serv_got_im(gc, who, body_str, flags, time(NULL));
	//		g_free(body_str);
	//	}

	nateon_message_destroy(msg);

	return 1;
}

	static unsigned int
nateon_send_typing(PurpleConnection *gc, const char *who, PurpleTypingState state)
{
	PurpleAccount *account;
	NateonSession *session;
	NateonSwitchBoard *swboard;
	NateonMessage *msg;

	account = purple_connection_get_account(gc);
	session = gc->proto_data;

	//	if (state == PURPLE_NOT_TYPING)
	//		return 0;

	if (!g_ascii_strcasecmp(who, purple_account_get_username(account)))
	{
		/* We'll just fake it, since we're sending to ourself. */
		serv_got_typing(gc, who, NATEON_TYPING_RECV_TIMEOUT, PURPLE_TYPING);

		return NATEON_TYPING_SEND_TIMEOUT;
	}

	swboard = nateon_session_find_swboard(session, who);

	if (swboard == NULL || !nateon_switchboard_can_send(swboard))
		return 0;

	swboard->flag |= NATEON_SB_FLAG_IM;

	msg = nateon_message_new(NATEON_MSG_TYPING);
	//	nateon_message_set_content_type(msg, "text/x-msmsgscontrol");
	//	nateon_message_set_flag(msg, 'U');
	//	nateon_message_set_attr(msg, "TypingUser",
	//						 purple_account_get_username(account));
	if (state == PURPLE_TYPING)
	{
		nateon_message_set_bin_data(msg, "TYPING 1", 8);
	}
	else 
	{
		nateon_message_set_bin_data(msg, "TYPING 0", 8);
	}

	nateon_switchboard_send_msg(swboard, msg, FALSE);

	nateon_message_destroy(msg);

	return NATEON_TYPING_SEND_TIMEOUT;
}

static void nateon_set_status(PurpleAccount *account, PurpleStatus *status)
{
	PurpleConnection *gc;
	NateonSession *session;

	purple_debug_info("nateon" ,"[%s]\n", __FUNCTION__);

	gc = purple_account_get_connection(account);

	if (gc != NULL)
	{
		purple_debug_info("nateon" ,"[%s] gc존재\n", __FUNCTION__);
		session = gc->proto_data;
		nateon_change_status(session);
	}
}

static void nateon_set_idle(PurpleConnection *gc, int idle)
{
	NateonSession *session;

	purple_debug_info("nateon" ,"[%s]\n", __FUNCTION__);

	session = gc->proto_data;

	nateon_change_status(session);
}

//#if 0
//static void
//fake_userlist_add_buddy(NateonUserList *userlist,
//					   const char *who, int list_id,
//					   const char *group_name)
//{
//	NateonUser *user;
//	static int group_id_c = 1;
//	int group_id;
//
//	group_id = -1;
//
//	if (group_name != NULL)
//	{
//		NateonGroup *group;
//		group = nateon_group_new(userlist, group_id_c, group_name);
//		group_id = group_id_c++;
//	}
//
//	user = nateon_userlist_find_user(userlist, who);
//
//	if (user == NULL)
//	{
//		user = nateon_user_new(userlist, who, NULL);
//		nateon_userlist_add_user(userlist, user);
//	}
//	else
//		if (user->list_op & (1 << list_id))
//		{
//			if (list_id == NATEON_LIST_FL)
//			{
//				if (group_id >= 0)
//					if (g_list_find(user->group_ids,
//									GINT_TO_POINTER(group_id)))
//						return;
//			}
//			else
//				return;
//		}
//
//	if (group_id >= 0)
//	{
//		user->group_ids = g_list_append(user->group_ids,
//										GINT_TO_POINTER(group_id));
//	}
//
//	user->list_op |= (1 << list_id);
//}
//#endif

	static void
nateon_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	NateonSession *session;
	NateonUserList *userlist;
	const char *who;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
	purple_debug_info("nateon", "[%s] group_name(%s)\n", __FUNCTION__, group->name);

	session = gc->proto_data;
	userlist = session->userlist;
	who = nateon_normalize(gc->account, buddy->name);
	g_free(buddy->name);
	buddy->name = g_strdup(who);

	if (!session->logged_in)
	{
#if 0
		fake_userlist_add_buddy(session->sync_userlist, who, NATEON_LIST_FL,
				group ? group->name : NULL);
#else
		purple_debug_error("nateon", "nateon_add_buddy called before connected\n");
#endif

		return;
	}

#if 0
	if (group != NULL && group->name != NULL)
		purple_debug_info("nateon", "nateon_add_buddy: %s, %s\n", who, group->name);
	else
		purple_debug_info("nateon", "nateon_add_buddy: %s\n", who);
#endif

#if 0
	/* Which is the max? */
	if (session->fl_users_count >= 150)
	{
		purple_debug_info("nateon", "Too many buddies\n");
		/* Buddy list full */
		/* TODO: purple should be notified of this */
		return;
	}
#endif

	/* XXX - Would group ever be NULL here?  I don't think so...
	 * shx: Yes it should; NATEON handles non-grouped buddies, and this is only
	 * internal. */
	nateon_userlist_add_buddy(userlist, who, NATEON_LIST_FL, group ? group->name : NULL);
}

	static void
nateon_rem_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group)
{
	NateonSession *session;
	NateonUserList *userlist;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	session = gc->proto_data;
	userlist = session->userlist;

	if (!session->logged_in)
		return;

	/* XXX - Does buddy->name need to be nateon_normalize'd here?  --KingAnt */
	nateon_userlist_rem_buddy(userlist, buddy->name, NATEON_LIST_FL, group->name);
}

	static void
nateon_add_permit(PurpleConnection *gc, const char *who)
{
	//	NateonSession *session;
	//	NateonUserList *userlist;
	//	NateonUser *user;
	//
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	//	session = gc->proto_data;
	//	userlist = session->userlist;
	//	user = nateon_userlist_find_user(userlist, who);
	//
	//	if (!session->logged_in)
	//		return;
	//
	//	if (user != NULL && user->list_op & NATEON_LIST_BL_OP)
	//		nateon_userlist_rem_buddy(userlist, who, NATEON_LIST_BL, NULL);
	//
	//	nateon_userlist_add_buddy(userlist, who, NATEON_LIST_AL, NULL);
}

	static void
nateon_add_deny(PurpleConnection *gc, const char *who)
{
	NateonSession *session;
	NateonUserList *userlist;
	NateonUser *user;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	session = gc->proto_data;
	userlist = session->userlist;
	user = nateon_userlist_find_user_with_name(userlist, who);

	if (!session->logged_in)
		return;

	if (user != NULL && user->list_op & NATEON_LIST_AL_OP)
		nateon_userlist_rem_buddy(userlist, who, NATEON_LIST_AL, NULL);

	nateon_userlist_add_buddy(userlist, who, NATEON_LIST_BL, NULL);
}

	static void
nateon_rem_permit(PurpleConnection *gc, const char *who)
{
	//	NateonSession *session;
	//	NateonUserList *userlist;
	//	NateonUser *user;
	//
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	//	session = gc->proto_data;
	//	userlist = session->userlist;
	//
	//	if (!session->logged_in)
	//		return;
	//
	//	user = nateon_userlist_find_user(userlist, who);
	//
	//	nateon_userlist_rem_buddy(userlist, who, NATEON_LIST_AL, NULL);
	//
	//	if (user != NULL && user->list_op & NATEON_LIST_RL_OP)
	//		nateon_userlist_add_buddy(userlist, who, NATEON_LIST_BL, NULL);
}

	static void
nateon_rem_deny(PurpleConnection *gc, const char *who)
{
	NateonSession *session;
	NateonUserList *userlist;
	NateonUser *user;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	session = gc->proto_data;
	userlist = session->userlist;

	if (!session->logged_in)
		return;

	user = nateon_userlist_find_user_with_name(userlist, who);

	nateon_userlist_rem_buddy(userlist, who, NATEON_LIST_BL, NULL);

	if (user != NULL && user->list_op & NATEON_LIST_RL_OP)
		nateon_userlist_add_buddy(userlist, who, NATEON_LIST_AL, NULL);
}

	static void
nateon_set_permit_deny(PurpleConnection *gc)
{
	//	PurpleAccount *account;
	//	NateonSession *session;
	//	NateonCmdProc *cmdproc;
	//
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	//	account = purple_connection_get_account(gc);
	//	session = gc->proto_data;
	//	cmdproc = session->notification->cmdproc;
	//
	//	if (account->perm_deny == PURPLE_PRIVACY_ALLOW_ALL ||
	//		account->perm_deny == PURPLE_PRIVACY_DENY_USERS)
	//	{
	//		nateon_cmdproc_send(cmdproc, "BLP", "%s", "AL");
	//	}
	//	else
	//	{
	//		nateon_cmdproc_send(cmdproc, "BLP", "%s", "BL");
	//	}
}

	static void
nateon_chat_invite(PurpleConnection *gc, int id, const char *msg,
		const char *who)
{
	//	NateonSession *session;
	//	NateonSwitchBoard *swboard;
	//
	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	//	session = gc->proto_data;
	//
	//	swboard = nateon_session_find_swboard_with_id(session, id);
	//
	//	if (swboard == NULL)
	//	{
	//		/* if we have no switchboard, everyone else left the chat already */
	//		swboard = nateon_switchboard_new(session);
	//		nateon_switchboard_request(swboard);
	//		swboard->chat_id = id;
	//		swboard->conv = purple_find_chat(gc, id);
	//	}
	//
	//	swboard->flag |= NATEON_SB_FLAG_IM;
	//
	//	nateon_switchboard_request_add_user(swboard, who);
}

	static void
nateon_chat_leave(PurpleConnection *gc, int id)
{
	//	NateonSession *session;
	//	NateonSwitchBoard *swboard;
	//	PurpleConversation *conv;
	//
	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	//	session = gc->proto_data;
	//
	//	swboard = nateon_session_find_swboard_with_id(session, id);
	//
	//	/* if swboard is NULL we were the only person left anyway */
	//	if (swboard == NULL)
	//		return;
	//
	//	conv = swboard->conv;
	//
	//	nateon_switchboard_release(swboard, NATEON_SB_FLAG_IM);
	//
	//	/* If other switchboards managed to associate themselves with this
	//	 * conv, make sure they know it's gone! */
	//	if (conv != NULL)
	//	{
	//		while ((swboard = nateon_session_find_swboard_with_conv(session, conv)) != NULL)
	//			swboard->conv = NULL;
	//	}
}

	static int
nateon_chat_send(PurpleConnection *gc, int id, const char *message, PurpleMessageFlags flags)
{
	//	PurpleAccount *account;
	//	NateonSession *session;
	//	NateonSwitchBoard *swboard;
	//	NateonMessage *msg;
	//	char *msgformat;
	//	char *msgtext;
	//
	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	//	account = purple_connection_get_account(gc);
	//	session = gc->proto_data;
	//	swboard = nateon_session_find_swboard_with_id(session, id);
	//
	//	if (swboard == NULL)
	//		return -EINVAL;
	//
	//	if (!swboard->ready)
	//		return 0;
	//
	//	swboard->flag |= NATEON_SB_FLAG_IM;
	//
	//	nateon_import_html(message, &msgformat, &msgtext);
	//
	//	if (strlen(msgtext) + strlen(msgformat) + strlen(VERSION) > 1564)
	//	{
	//		g_free(msgformat);
	//		g_free(msgtext);
	//
	//		return -E2BIG;
	//	}
	//
	//	msg = nateon_message_new_plain(msgtext);
	//	nateon_message_set_attr(msg, "X-MMS-IM-Format", msgformat);
	//	nateon_switchboard_send_msg(swboard, msg, FALSE);
	//	nateon_message_destroy(msg);
	//
	//	g_free(msgformat);
	//	g_free(msgtext);
	//
	//	serv_got_chat_in(gc, id, purple_account_get_username(account), 0,
	//					 message, time(NULL));
	//
	return 0;
}

//static void
//nateon_keepalive(PurpleConnection *gc)
//{
//	NateonSession *session;
//
//	session = gc->proto_data;
//
//	if (!session->http_method)
//	{
//		NateonCmdProc *cmdproc;
//
//		cmdproc = session->notification->cmdproc;
//
//		nateon_cmdproc_send_quick(cmdproc, "PNG", NULL, NULL);
//	}
//}

	static void
nateon_group_buddy(PurpleConnection *gc, const char *who,
		const char *old_group_name, const char *new_group_name)
{
	NateonSession *session;
	NateonUserList *userlist;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	purple_debug_info("nateon", "who:%s, old:%s, new:%s\n", who, old_group_name, new_group_name);

	session = gc->proto_data;
	userlist = session->userlist;

	nateon_userlist_move_buddy(userlist, who, old_group_name, new_group_name);
}

	static void
nateon_rename_group(PurpleConnection *gc, const char *old_name,
		PurpleGroup *group, GList *moved_buddies)
{
	NateonSession *session;
	NateonUserList *userlist;
	NateonCmdProc *cmdproc;
	int old_gid;
	const char *enc_new_group_name;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	session = gc->proto_data;
	userlist = session->userlist;
	cmdproc = session->notification->cmdproc;
	enc_new_group_name = purple_strreplace(group->name, " ", "%20");

	old_gid = nateon_userlist_find_group_id(userlist, old_name);

	purple_debug_info("nateon", "[%s] old_gid(%d)\n", __FUNCTION__, old_gid);

	if (old_gid == 0) // '기타'그룹
	{
		GList *l;
		PurpleGroup *g;

		for (l = userlist->users; l != NULL; l = l->next)
		{
			NateonUser *user = (NateonUser *)l->data;

			if (g_list_find(user->group_ids, GINT_TO_POINTER(0)) != NULL)
			{
				nateon_userlist_move_buddy(userlist, user->account_name, old_name, group->name);
			}
		}

		nateon_group_new(userlist, 0, old_name);
		g = purple_group_new(old_name);
		purple_blist_add_group(g, NULL);
	}
	else if (old_gid > 0)
	{
		nateon_cmdproc_send(cmdproc, "RENG", "0 %d %s", old_gid, enc_new_group_name);
	}
	else
	{
		nateon_cmdproc_send(cmdproc, "ADDG", "0 %s", enc_new_group_name);
	}
}

	static void
nateon_convo_closed(PurpleConnection *gc, const char *who)
{
	NateonSession *session;
	NateonSwitchBoard *swboard;
	PurpleConversation *conv;

	session = gc->proto_data;

	swboard = nateon_session_find_swboard(session, who);

	/*
	 * Don't perform an assertion here. If swboard is NULL, then the
	 * switchboard was either closed by the other party, or the person
	 * is talking to himself.
	 */
	if (swboard == NULL)
		return;

	conv = swboard->conv;

	nateon_switchboard_release(swboard, NATEON_SB_FLAG_IM);

	/* If other switchboards managed to associate themselves with this
	 * conv, make sure they know it's gone! */
	if (conv != NULL)
	{
		while ((swboard = nateon_session_find_swboard_with_conv(session, conv)) != NULL)
			swboard->conv = NULL;
	}
}

//static void
//nateon_set_buddy_icon(PurpleConnection *gc, const char *filename)
//{
//	NateonSession *session;
//	NateonUser *user;
//
//	session = gc->proto_data;
//	user = session->user;
//
//	nateon_user_set_buddy_icon(user, filename);
//
//	nateon_change_status(session);
//}

	static void
nateon_remove_group(PurpleConnection *gc, PurpleGroup *group)
{
	NateonSession *session;
	NateonCmdProc *cmdproc;
	int group_id;

	session = gc->proto_data;
	cmdproc = session->notification->cmdproc;

	if ((group_id = nateon_userlist_find_group_id(session->userlist, group->name)) >= 0)
	{
		nateon_cmdproc_send(cmdproc, "RMVG", "0 %d", group_id);
	}
}

//static char *
//nateon_tooltip_info_text(NateonGetInfoData *info_data)
//{
//	GString *s;
//	PurpleBuddy *b;
//
//	s = g_string_sized_new(80); /* wild guess */
//
//	b = purple_find_buddy(purple_connection_get_account(info_data->gc),
//						info_data->name);
//
//	if (b)
//	{
//		PurplePresence *presence;
//		GString *str = g_string_new("");
//		char *tmp;
//
//		presence = purple_buddy_get_presence(b);
//
//		if (b->alias && b->alias[0])
//		{
//			char *aliastext = g_markup_escape_text(b->alias, -1);
//			g_string_append_printf(s, _("<b>Alias:</b> %s<br>"), aliastext);
//			g_free(aliastext);
//		}
//
//		if (b->server_alias)
//		{
//			char *nicktext = g_markup_escape_text(b->server_alias, -1);
//			g_string_append_printf(s, _("<b>%s:</b> "), _("Nickname"));
//			g_string_append_printf(s, "<font sml=\"nateon\">%s</font><br>",
//					nicktext);
//			g_free(nicktext);
//		}
//
//		nateon_tooltip_text(b, str, TRUE);
//		tmp = purple_strreplace((*str->str == '\n' ? str->str + 1 : str->str),
//							  "\n", "<br>");
//		g_string_free(str, TRUE);
//		g_string_append_printf(s, "%s<br>", tmp);
//		g_free(tmp);
//	}
//
//	return g_string_free(s, FALSE);
//}
//
//#if PHOTO_SUPPORT
//
//static char *
//nateon_get_photo_url(const char *url_text)
//{
//	char *p, *q;
//
//	if ((p = strstr(url_text, " contactparams:photopreauthurl=\"")) != NULL)
//	{
//		p += strlen(" contactparams:photopreauthurl=\"");
//	}
//
//	if (p && (strncmp(p, "http://", 8) == 0) && ((q = strchr(p, '"')) != NULL))
//			return g_strndup(p, q - p);
//
//	return NULL;
//}
//
//static void nateon_got_photo(void *data, const char *url_text, size_t len);
//
//#endif
//
//#if 0
//static char *nateon_info_date_reformat(const char *field, size_t len)
//{
//	char *tmp = g_strndup(field, len);
//	time_t t = purple_str_to_time(tmp, FALSE, NULL, NULL, NULL);
//
//	g_free(tmp);
//	return g_strdup(purple_date_format_short(localtime(&t)));
//}
//#endif
/*
#define NATEON_GOT_INFO_GET_FIELD(a, b) \
found = purple_markup_extract_info_field(stripped, stripped_len, s, \
"\n" a "\t", 0, "\n", 0, "Undisclosed", b, 0, NULL, NULL); \
if (found) \
sect_info = TRUE;
*/
//static void
//nateon_got_info(void *data, const char *url_text, size_t len)
//{
//	NateonGetInfoData *info_data = (NateonGetInfoData *)data;
//	char *stripped, *p, *q;
//	char buf[1024];
//	char *tooltip_text;
//	char *user_url = NULL;
//	gboolean found;
//	gboolean has_info = FALSE;
//	gboolean sect_info = FALSE;
//	const char* title = NULL;
//	char *url_buffer;
//	char *personal = NULL;
//	char *business = NULL;
//	GString *s, *s2;
//	int stripped_len;
//#if PHOTO_SUPPORT
//	char *photo_url_text = NULL;
//	NateonGetInfoStepTwoData *info2_data = NULL;
//#endif
//
//	purple_debug_info("nateon", "In nateon_got_info\n");
//
//	/* Make sure the connection is still valid */
//	if (g_list_find(purple_connections_get_all(), info_data->gc) == NULL)
//	{
//		purple_debug_warning("nateon", "invalid connection. ignoring buddy info.\n");
//		g_free(info_data->name);
//		g_free(info_data);
//		return;
//	}
//
//	tooltip_text = nateon_tooltip_info_text(info_data);
//	title = _("NATEON Profile");
//
//	if (url_text == NULL || strcmp(url_text, "") == 0)
//	{
//		g_snprintf(buf, 1024, "<html><body>%s<b>%s</b></body></html>",
//				tooltip_text, _("Error retrieving profile"));
//
//		purple_notify_userinfo(info_data->gc, info_data->name, buf, NULL, NULL);
//
//		g_free(tooltip_text);
//		return;
//	}
//
//	url_buffer = g_strdup(url_text);
//
//	/* If they have a homepage link, NATEON masks it such that we need to
//	 * fetch the url out before purple_markup_strip_html() nukes it */
//	/* I don't think this works with the new spaces profiles - Stu 3/2/06 */
//	if ((p = strstr(url_text,
//			"Take a look at my </font><A class=viewDesc title=\"")) != NULL)
//	{
//		p += 50;
//
//		if ((q = strchr(p, '"')) != NULL)
//			user_url = g_strndup(p, q - p);
//	}
//
//	/*
//	 * purple_markup_strip_html() doesn't strip out character entities like &nbsp;
//	 * and &#183;
//	 */
//	while ((p = strstr(url_buffer, "&nbsp;")) != NULL)
//	{
//		*p = ' '; /* Turn &nbsp;'s into ordinary blanks */
//		p += 1;
//		memmove(p, p + 5, strlen(p + 5));
//		url_buffer[strlen(url_buffer) - 5] = '\0';
//	}
//
//	while ((p = strstr(url_buffer, "&#183;")) != NULL)
//	{
//		memmove(p, p + 6, strlen(p + 6));
//		url_buffer[strlen(url_buffer) - 6] = '\0';
//	}
//
//	/* Nuke the nasty \r's that just get in the way */
//	purple_str_strip_char(url_buffer, '\r');
//
//	/* NATEON always puts in &#39; for apostrophes...replace them */
//	while ((p = strstr(url_buffer, "&#39;")) != NULL)
//	{
//		*p = '\'';
//		memmove(p + 1, p + 5, strlen(p + 5));
//		url_buffer[strlen(url_buffer) - 4] = '\0';
//	}
//
//	/* Nuke the html, it's easier than trying to parse the horrid stuff */
//	stripped = purple_markup_strip_html(url_buffer);
//	stripped_len = strlen(stripped);
//
//	purple_debug_misc("nateon", "stripped = %p\n", stripped);
//	purple_debug_misc("nateon", "url_buffer = %p\n", url_buffer);
//
//	/* Gonna re-use the memory we've already got for url_buffer */
//	/* No we're not. */
//	s = g_string_sized_new(strlen(url_buffer));
//	s2 = g_string_sized_new(strlen(url_buffer));
//
//	/* Extract their Name and put it in */
//	NATEON_GOT_INFO_GET_FIELD("Name", _("Name"))
//
//	/* General */
//	NATEON_GOT_INFO_GET_FIELD("Nickname", _("Nickname"));
//	NATEON_GOT_INFO_GET_FIELD("Age", _("Age"));
//	NATEON_GOT_INFO_GET_FIELD("Gender", _("Gender"));
//	NATEON_GOT_INFO_GET_FIELD("Occupation", _("Occupation"));
//	NATEON_GOT_INFO_GET_FIELD("Location", _("Location"));
//
//	/* Extract their Interests and put it in */
//	found = purple_markup_extract_info_field(stripped, stripped_len, s,
//			"\nInterests\t", 0, " (/default.aspx?page=searchresults", 0,
//			"Undisclosed", _("Hobbies and Interests") /* _("Interests") */,
//			0, NULL, NULL);
//
//	if (found)
//		sect_info = TRUE;
//
//	NATEON_GOT_INFO_GET_FIELD("More about me", _("A Little About Me"));
//
//	if (sect_info)
//	{
//		/* trim off the trailing "<br>\n" */
//		g_string_truncate(s, strlen(s->str) - 5);
//		g_string_append_printf(s2, _("%s<b>General</b><br>%s"),
//							   (*tooltip_text) ? "<hr>" : "", s->str);
//		s = g_string_truncate(s, 0);
//		has_info = TRUE;
//		sect_info = FALSE;
//	}
//
//
//	/* Social */
//	NATEON_GOT_INFO_GET_FIELD("Marital status", _("Marital Status"));
//	NATEON_GOT_INFO_GET_FIELD("Interested in", _("Interests"));
//	NATEON_GOT_INFO_GET_FIELD("Pets", _("Pets"));
//	NATEON_GOT_INFO_GET_FIELD("Hometown", _("Hometown"));
//	NATEON_GOT_INFO_GET_FIELD("Places lived", _("Places Lived"));
//	NATEON_GOT_INFO_GET_FIELD("Fashion", _("Fashion"));
//	NATEON_GOT_INFO_GET_FIELD("Humor", _("Humor"));
//	NATEON_GOT_INFO_GET_FIELD("Music", _("Music"));
//	NATEON_GOT_INFO_GET_FIELD("Favorite quote", _("Favorite Quote"));
//
//	if (sect_info)
//	{
//		g_string_append_printf(s2, _("%s<b>Social</b><br>%s"), has_info ? "<br><hr>" : "", s->str);
//		s = g_string_truncate(s, 0);
//		has_info = TRUE;
//		sect_info = FALSE;
//	}
//
//	/* Contact Info */
//	/* Personal */
//	NATEON_GOT_INFO_GET_FIELD("Name", _("Name"));
//	NATEON_GOT_INFO_GET_FIELD("Significant other", _("Significant Other"));
//	NATEON_GOT_INFO_GET_FIELD("Home phone", _("Home Phone"));
//	NATEON_GOT_INFO_GET_FIELD("Home phone 2", _("Home Phone 2"));
//	NATEON_GOT_INFO_GET_FIELD("Home address", _("Home Address"));
//	NATEON_GOT_INFO_GET_FIELD("Personal Mobile", _("Personal Mobile"));
//	NATEON_GOT_INFO_GET_FIELD("Home fax", _("Home Fax"));
//	NATEON_GOT_INFO_GET_FIELD("Personal e-mail", _("Personal E-Mail"));
//	NATEON_GOT_INFO_GET_FIELD("Personal IM", _("Personal IM"));
//	NATEON_GOT_INFO_GET_FIELD("Birthday", _("Birthday"));
//	NATEON_GOT_INFO_GET_FIELD("Anniversary", _("Anniversary"));
//	NATEON_GOT_INFO_GET_FIELD("Notes", _("Notes"));
//
//	if (sect_info)
//	{
//		personal = g_strdup_printf(_("<br><b>Personal</b><br>%s"), s->str);
//		s = g_string_truncate(s, 0);
//		sect_info = FALSE;
//	}
//
//	/* Business */
//	NATEON_GOT_INFO_GET_FIELD("Name", _("Name"));
//	NATEON_GOT_INFO_GET_FIELD("Job title", _("Job Title"));
//	NATEON_GOT_INFO_GET_FIELD("Company", _("Company"));
//	NATEON_GOT_INFO_GET_FIELD("Department", _("Department"));
//	NATEON_GOT_INFO_GET_FIELD("Profession", _("Profession"));
//	NATEON_GOT_INFO_GET_FIELD("Work phone 1", _("Work Phone"));
//	NATEON_GOT_INFO_GET_FIELD("Work phone 2", _("Work Phone 2"));
//	NATEON_GOT_INFO_GET_FIELD("Work address", _("Work Address"));
//	NATEON_GOT_INFO_GET_FIELD("Work mobile", _("Work Mobile"));
//	NATEON_GOT_INFO_GET_FIELD("Work pager", _("Work Pager"));
//	NATEON_GOT_INFO_GET_FIELD("Work fax", _("Work Fax"));
//	NATEON_GOT_INFO_GET_FIELD("Work e-mail", _("Work E-Mail"));
//	NATEON_GOT_INFO_GET_FIELD("Work IM", _("Work IM"));
//	NATEON_GOT_INFO_GET_FIELD("Start date", _("Start Date"));
//	NATEON_GOT_INFO_GET_FIELD("Notes", _("Notes"));
//
//	if (sect_info)
//	{
//		business = g_strdup_printf(_("<br><b>Business</b><br>%s"), s->str);
//		s = g_string_truncate(s, 0);
//		sect_info = FALSE;
//	}
//
//	if ((personal != NULL) || (business != NULL))
//	{
//		/* trim off the trailing "<br>\n" */
//		g_string_truncate(s, strlen(s->str) - 5);
//
//		has_info = TRUE;
//		g_string_append_printf(s2, _("<hr><b>Contact Info</b>%s%s"),
//							   personal ? personal : "",
//							   business ? business : "");
//	}
//
//	g_free(personal);
//	g_free(business);
//	g_string_free(s, TRUE);
//	s = s2;
//
//#if 0 /* these probably don't show up any more */
//	/*
//	 * The fields, 'A Little About Me', 'Favorite Things', 'Hobbies
//	 * and Interests', 'Favorite Quote', and 'My Homepage' may or may
//	 * not appear, in any combination. However, they do appear in
//	 * certain order, so we can successively search to pin down the
//	 * distinct values.
//	 */
//
//	/* Check if they have A Little About Me */
//	found = purple_markup_extract_info_field(stripped, stripped_len, s,
//			" A Little About Me \n\n", 0, "Favorite Things", '\n', NULL,
//			_("A Little About Me"), 0, NULL, NULL);
//
//	if (!found)
//	{
//		found = purple_markup_extract_info_field(stripped, stripped_len, s,
//				" A Little About Me \n\n", 0, "Hobbies and Interests", '\n',
//				NULL, _("A Little About Me"), 0, NULL, NULL);
//	}
//
//	if (!found)
//	{
//		found = purple_markup_extract_info_field(stripped, stripped_len, s,
//				" A Little About Me \n\n", 0, "Favorite Quote", '\n', NULL,
//				_("A Little About Me"), 0, NULL, NULL);
//	}
//
//	if (!found)
//	{
//		found = purple_markup_extract_info_field(stripped, stripped_len, s,
//				" A Little About Me \n\n", 0, "My Homepage \n\nTake a look",
//				'\n',
//				NULL, _("A Little About Me"), 0, NULL, NULL);
//	}
//
//	if (!found)
//	{
//		purple_markup_extract_info_field(stripped, stripped_len, s,
//				" A Little About Me \n\n", 0, "last updated", '\n', NULL,
//				_("A Little About Me"), 0, NULL, NULL);
//	}
//
//	if (found)
//		has_info = TRUE;
//
//	/* Check if they have Favorite Things */
//	found = purple_markup_extract_info_field(stripped, stripped_len, s,
//			" Favorite Things \n\n", 0, "Hobbies and Interests", '\n', NULL,
//			_("Favorite Things"), 0, NULL, NULL);
//
//	if (!found)
//	{
//		found = purple_markup_extract_info_field(stripped, stripped_len, s,
//				" Favorite Things \n\n", 0, "Favorite Quote", '\n', NULL,
//				_("Favorite Things"), 0, NULL, NULL);
//	}
//
//	if (!found)
//	{
//		found = purple_markup_extract_info_field(stripped, stripped_len, s,
//				" Favorite Things \n\n", 0, "My Homepage \n\nTake a look", '\n',
//				NULL, _("Favorite Things"), 0, NULL, NULL);
//	}
//
//	if (!found)
//	{
//		purple_markup_extract_info_field(stripped, stripped_len, s,
//				" Favorite Things \n\n", 0, "last updated", '\n', NULL,
//				_("Favorite Things"), 0, NULL, NULL);
//	}
//
//	if (found)
//		has_info = TRUE;
//
//	/* Check if they have Hobbies and Interests */
//	found = purple_markup_extract_info_field(stripped, stripped_len, s,
//			" Hobbies and Interests \n\n", 0, "Favorite Quote", '\n', NULL,
//			_("Hobbies and Interests"), 0, NULL, NULL);
//
//	if (!found)
//	{
//		found = purple_markup_extract_info_field(stripped, stripped_len, s,
//				" Hobbies and Interests \n\n", 0, "My Homepage \n\nTake a look",
//				'\n', NULL, _("Hobbies and Interests"), 0, NULL, NULL);
//	}
//
//	if (!found)
//	{
//		purple_markup_extract_info_field(stripped, stripped_len, s,
//				" Hobbies and Interests \n\n", 0, "last updated", '\n', NULL,
//				_("Hobbies and Interests"), 0, NULL, NULL);
//	}
//
//	if (found)
//		has_info = TRUE;
//
//	/* Check if they have Favorite Quote */
//	found = purple_markup_extract_info_field(stripped, stripped_len, s,
//			"Favorite Quote \n\n", 0, "My Homepage \n\nTake a look", '\n', NULL,
//			_("Favorite Quote"), 0, NULL, NULL);
//
//	if (!found)
//	{
//		purple_markup_extract_info_field(stripped, stripped_len, s,
//				"Favorite Quote \n\n", 0, "last updated", '\n', NULL,
//				_("Favorite Quote"), 0, NULL, NULL);
//	}
//
//	if (found)
//		has_info = TRUE;
//
//	/* Extract the last updated date and put it in */
//	found = purple_markup_extract_info_field(stripped, stripped_len, s,
//			" last updated:", 1, "\n", 0, NULL, _("Last Updated"), 0,
//			NULL, nateon_info_date_reformat);
//
//	if (found)
//		has_info = TRUE;
//#endif
//
//	/* If we were able to fetch a homepage url earlier, stick it in there */
//	if (user_url != NULL)
//	{
//		g_snprintf(buf, sizeof(buf),
//				   "<b>%s:</b><br><a href=\"%s\">%s</a><br>\n",
//				   _("Homepage"), user_url, user_url);
//
//		g_string_append(s, buf);
//
//		g_free(user_url);
//
//		has_info = TRUE;
//	}
//
//	if (!has_info)
//	{
//		/* NATEON doesn't actually distinguish between "unknown member" and
//		 * a known member with an empty profile. Try to explain this fact.
//		 * Note that if we have a nonempty tooltip_text, we know the user
//		 * exists.
//		 */
//		/* This doesn't work with the new spaces profiles - Stu 3/2/06
//		char *p = strstr(url_buffer, "Unknown Member </TITLE>");
//		 * This might not work for long either ... */
//		char *p = strstr(url_buffer, "form id=\"SpacesSearch\" name=\"SpacesSearch\"");
//		PurpleBuddy *b = purple_find_buddy
//				(purple_connection_get_account(info_data->gc), info_data->name);
//		g_string_append_printf(s, "<br><b>%s</b><br>%s<br><br>",
//				_("Error retrieving profile"),
//				((p && b)?
//					_("The user has not created a public profile."):
//				 p? _("NATEON reported not being able to find the user's profile. "
//					  "This either means that the user does not exist, "
//					  "or that the user exists "
//					  "but has not created a public profile."):
//					_("Purple could not find "	/* This should never happen */
//					  "any information in the user's profile. "
//					  "The user most likely does not exist.")));
//	}
//	/* put a link to the actual profile URL */
//	g_string_append_printf(s, _("<hr><b>%s:</b> "), _("Profile URL"));
//	g_string_append_printf(s, "<br><a href=\"%s%s\">%s%s</a><br>",
//			PROFILE_URL, info_data->name, PROFILE_URL, info_data->name);
//
//	/* Finish it off, and show it to them */
//	g_string_append(s, "</body></html>\n");
//
//#if PHOTO_SUPPORT
//	/* Find the URL to the photo; must be before the marshalling [Bug 994207] */
//	photo_url_text = nateon_get_photo_url(url_text);
//
//	/* Marshall the existing state */
//	info2_data = g_malloc0(sizeof(NateonGetInfoStepTwoData));
//	info2_data->info_data = info_data;
//	info2_data->stripped = stripped;
//	info2_data->url_buffer = url_buffer;
//	info2_data->s = s;
//	info2_data->photo_url_text = photo_url_text;
//	info2_data->tooltip_text = tooltip_text;
//	info2_data->title = title;
//
//	/* Try to put the photo in there too, if there's one */
//	if (photo_url_text)
//	{
//		purple_url_fetch(photo_url_text, FALSE, NULL, FALSE, nateon_got_photo,
//					   info2_data);
//	}
//	else
//	{
//		/* Emulate a callback */
//		nateon_got_photo(info2_data, NULL, 0);
//	}
//}
//
//static void
//nateon_got_photo(void *data, const char *url_text, size_t len)
//{
//	NateonGetInfoStepTwoData *info2_data = (NateonGetInfoStepTwoData *)data;
//	int id = -1;
//
//	/* Unmarshall the saved state */
//	NateonGetInfoData *info_data = info2_data->info_data;
//	char *stripped = info2_data->stripped;
//	char *url_buffer = info2_data->url_buffer;
//	GString *s = info2_data->s;
//	char *photo_url_text = info2_data->photo_url_text;
//	char *tooltip_text = info2_data->tooltip_text;
//
//	/* Make sure the connection is still valid if we got here by fetching a photo url */
//	if (url_text &&
//		g_list_find(purple_connections_get_all(), info_data->gc) == NULL)
//	{
//		purple_debug_warning("nateon", "invalid connection. ignoring buddy photo info.\n");
//		g_free(stripped);
//		g_free(url_buffer);
//		g_string_free(s, TRUE);
//		g_free(tooltip_text);
//		g_free(info_data->name);
//		g_free(info_data);
//		g_free(photo_url_text);
//		g_free(info2_data);
//
//		return;
//	}
//
//	/* Try to put the photo in there too, if there's one and is readable */
//	if (data && url_text && len != 0)
//	{
//		if (strstr(url_text, "400 Bad Request")
//			|| strstr(url_text, "403 Forbidden")
//			|| strstr(url_text, "404 Not Found"))
//		{
//
//			purple_debug_info("nateon", "Error getting %s: %s\n",
//					photo_url_text, url_text);
//		}
//		else
//		{
//			char buf[1024];
//			purple_debug_info("nateon", "%s is %d bytes\n", photo_url_text, len);
//			id = purple_imgstore_add(url_text, len, NULL);
//			g_snprintf(buf, sizeof(buf), "<img id=\"%d\"><br>", id);
//			g_string_prepend(s, buf);
//		}
//	}
//
//	/* We continue here from nateon_got_info, as if nothing has happened */
//#endif
//
//	g_string_prepend(s, tooltip_text);
//	purple_notify_userinfo(info_data->gc, info_data->name, s->str, NULL, NULL);
//
//	g_free(stripped);
//	g_free(url_buffer);
//	g_string_free(s, TRUE);
//	g_free(tooltip_text);
//	g_free(info_data->name);
//	g_free(info_data);
//#if PHOTO_SUPPORT
//	g_free(photo_url_text);
//	g_free(info2_data);
//	if (id != -1)
//		purple_imgstore_unref(id);
//#endif
//}

	static void
nateon_get_info(PurpleConnection *gc, const char *name)
{
	//	NateonGetInfoData *data;
	//	char *url;
	//
	//	data       = g_new0(NateonGetInfoData, 1);
	//	data->gc   = gc;
	//	data->name = g_strdup(name);
	//
	//	url = g_strdup_printf("%s%s", PROFILE_URL, name);
	//
	//	purple_url_fetch(url, FALSE,
	//				   "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)",
	//				   TRUE, nateon_got_info, data);
	//
	//	g_free(url);
}

static gboolean nateon_load(PurplePlugin *plugin)
{
	nateon_notification_init();
	nateon_switchboard_init();
	nateon_sync_init();

	return TRUE;
}

static gboolean nateon_unload(PurplePlugin *plugin)
{
	nateon_notification_end();
	nateon_switchboard_end();
	nateon_sync_end();

	return TRUE;
}

static PurplePluginProtocolInfo prpl_info =
{
	0, //OPT_PROTO_MAIL_CHECK,
	NULL,					/* user_splits */
	NULL,					/* protocol_options */
	{ "bmp,jpg", 0, 0, 500, 500, 0, PURPLE_ICON_SCALE_SEND},    /* icon_spec */
	nateon_list_icon,			/* list_icon */
	NULL,					/* list_emblems */
	nateon_status_text,			/* status_text */
	nateon_tooltip_text,			/* tooltip_text */
	nateon_status_types,			/* away_states */
	nateon_blist_node_menu,			/* blist_node_menu */
	NULL,					/* chat_info */
	NULL,					/* chat_info_defaults */
	nateon_login,				/* login */
	nateon_close,				/* close */
	nateon_send_im,				/* send_im */
	NULL,					/* set_info */
	nateon_send_typing,			/* send_typing */
	nateon_get_info,			/* get_info */
	nateon_set_status,			/* set_away */
	nateon_set_idle,			/* set_idle */
	NULL,					/* change_passwd */
	nateon_add_buddy,			/* add_buddy */
	NULL,					/* add_buddies */
	nateon_rem_buddy,			/* remove_buddy */
	NULL,					/* remove_buddies */
	nateon_add_permit,			/* add_permit */
	nateon_add_deny,			/* add_deny */
	nateon_rem_permit,			/* rem_permit */
	nateon_rem_deny,			/* rem_deny */
	nateon_set_permit_deny,			/* set_permit_deny */
	NULL,					/* join_chat */
	NULL,					/* reject chat invite */
	NULL,					/* get_chat_name */
	NULL, //nateon_chat_invite,			/* chat_invite */
	NULL, //nateon_chat_leave,			/* chat_leave */
	NULL,					/* chat_whisper */
	nateon_chat_send,			/* chat_send */
	NULL, //nateon_keepalive,			/* keepalive */
	NULL,					/* register_user */
	NULL,					/* get_cb_info */
	NULL,					/* get_cb_away */
	NULL,					/* alias_buddy */
	nateon_group_buddy,			/* group_buddy */
	nateon_rename_group,			/* rename_group */
	NULL,					/* buddy_free */
	nateon_convo_closed,			/* convo_closed */
	nateon_normalize,			/* normalize */
	NULL, //nateon_set_buddy_icon,		/* set_buddy_icon */
	nateon_remove_group,			/* remove_group */
	NULL,					/* get_cb_real_name */
	NULL,					/* set_chat_topic */
	NULL,					/* find_blist_chat */
	NULL,					/* roomlist_get_list */
	NULL,					/* roomlist_cancel */
	NULL,					/* roomlist_expand_category */
	nateon_can_receive_file,	/* can_receive_file */
	nateon_send_file,			/* send_file */
	NULL, //nateon_new_xfer,			/* new_xfer */
	NULL,					/* offline_message */
	NULL,					/* whiteboard_prpl_ops */
	NULL,                                   /* send_raw */
	NULL,                                   /* roomlist_room_serialize */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_PROTOCOL,			/**< type           */
	NULL,					/**< ui_requirement */
	0,					/**< flags          */
	NULL,					/**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,		/**< priority       */

	"prpl-nateon",				/**< id             */
	"NateOn",				/**< name           */
	VERSION,				/**< version        */
	/**  summary        */
	N_("NateOn Protocol Plugin"),
	/**  description    */
	N_("NateOn Protocol Plugin"),
	"Hansun Lee <hansun.lee@gmail.com>",	/**< author         */
	"http://nateon.haz3.com",		/**< homepage       */

	nateon_load,				/**< load           */
	nateon_unload,				/**< unload         */
	NULL,					/**< destroy        */

	NULL,					/**< ui_info        */
	&prpl_info, 				/**< extra_info     */
	NULL, 					/**< prefs_info     */
	nateon_actions,

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

	static void
init_plugin(PurplePlugin *plugin)
{
	PurpleAccountOption *option;

#ifdef ENABLE_NLS
	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	option = purple_account_option_string_new(_("Server"), "server", NATEON_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new(_("Port"), "port", NATEON_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new(_("Use PRS Method"), "prs_method", FALSE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new(_("PRS Method Server"), "prs_method_server", NATEON_PRS_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
	//	option = purple_account_option_bool_new(_("Show custom smileys"),
	//										  "custom_smileys", TRUE);
	//	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,

	purple_prefs_remove("/plugins/prpl/nateon");
}

PURPLE_INIT_PLUGIN(nateon, init_plugin, info);
/* vim: set ts=4 sw=4: */
