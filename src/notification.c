/**
 * @file notification.c Notification server functions
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
#include "notification.h"
#include "xfer.h"
#include "state.h"
//#include "error.h"
//#include "utils.h"
//#include "page.h"
//
#include "userlist.h"
#include "sync.h"
//#include "slplink.h"

static NateonTable *cbs_table;

/**************************************************************************
 * Main
 **************************************************************************/

static void
destroy_cb(NateonServConn *servconn)
{
	NateonNotification *notification;

	notification = servconn->cmdproc->data;
	g_return_if_fail(notification != NULL);

	nateon_notification_destroy(notification);
}

NateonNotification *
nateon_notification_new(NateonSession *session)
{
	NateonNotification *notification;
	NateonServConn *servconn;

	g_return_val_if_fail(session != NULL, NULL);

	notification = g_new0(NateonNotification, 1);

	notification->session = session;
	notification->servconn = servconn = nateon_servconn_new(session, NATEON_SERVCONN_NS);
	nateon_servconn_set_destroy_cb(servconn, destroy_cb);

	notification->cmdproc = servconn->cmdproc;
	notification->cmdproc->data = notification;
	notification->cmdproc->cbs_table = cbs_table;

	return notification;
}

void
nateon_notification_destroy(NateonNotification *notification)
{
	notification->cmdproc->data = NULL;

	nateon_servconn_set_destroy_cb(notification->servconn, NULL);

	nateon_servconn_destroy(notification->servconn);

	g_free(notification);
}

/**************************************************************************
 * Connect
 **************************************************************************/

static void
connect_cb(NateonServConn *servconn)
{
	NateonCmdProc *cmdproc;
	NateonSession *session;
	PurpleAccount *account;
	char *cmd = NULL;

	g_return_if_fail(servconn != NULL);

	cmdproc = servconn->cmdproc;
	session = servconn->session;
	account = session->account;


	if (session->login_step == NATEON_LOGIN_STEP_START)
	{
		cmd = g_strdup_printf("%1.3f %d.0", session->protocol_ver, NATEON_MAJOR_VERSION);

		nateon_session_set_login_step(session, NATEON_LOGIN_STEP_HANDSHAKE);

		//nateon_cmdproc_send(cmdproc, "PVER", "%s", cmd);
		nateon_cmdproc_send(cmdproc, "PVER", "3.615 3.0");

	}
	else
	{
		const char *username = purple_account_get_username(account);
		const char *password = purple_account_get_password(account);
		char *token;
		PurpleCipher *cipher;
		PurpleCipherContext *context;
		guchar digest[16];
		char buf[33];
		int i;

		token = g_strdup(username);
		if (strstr(token, "@nate.com"))	strtok(token, "@");

		cipher = purple_ciphers_find_cipher("md5");
		context = purple_cipher_context_new(cipher, NULL);

		purple_cipher_context_append(context, (const guchar *)password, strlen(password));
		purple_cipher_context_append(context, (const guchar *)token, strlen(token));

		purple_cipher_context_digest(context, sizeof(digest), digest, NULL);
		purple_cipher_context_destroy(context);

		g_free(token);

		for (i = 0; i < 16; i++)
			g_snprintf(buf + (i*2), 3, "%02x", digest[i]);

		cmd = g_strdup_printf("%s %s MD5 %1.3f UTF8", username, buf, session->protocol_ver);
		nateon_session_set_login_step(session, NATEON_LOGIN_STEP_AUTH_START);

		nateon_cmdproc_send(cmdproc, "LSIN", "%s", cmd);
	}


	g_free(cmd);
}

gboolean
nateon_notification_connect(NateonNotification *notification, const char *host, int port)
{
	NateonServConn *servconn;

	g_return_val_if_fail(notification != NULL, FALSE);

	servconn = notification->servconn;

	nateon_servconn_set_connect_cb(servconn, connect_cb);
	notification->in_use = nateon_servconn_connect(servconn, host, port);

	return notification->in_use;
}

void
nateon_notification_disconnect(NateonNotification *notification)
{
	g_return_if_fail(notification != NULL);
	g_return_if_fail(notification->in_use);

	nateon_servconn_disconnect(notification->servconn);

	notification->in_use = FALSE;
}

/**************************************************************************
 * Util
 **************************************************************************/

static void
group_error_helper(NateonSession *session, const char *msg, int group_id, int error)
{
	PurpleAccount *account;
	PurpleConnection *gc;
	char *reason = NULL;
	char *title = NULL;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	account = session->account;
	gc = purple_account_get_connection(account);

	if (error == 304)
	{

		if (group_id == 0)
		{
			purple_debug_info("nateon", "[%s] 기본그룹의 명칭을 변경하려고 함.\n", __FUNCTION__);
			return;
		}
		else
		{
			const char *group_name;
			group_name = nateon_userlist_find_group_name(session->userlist, group_id);
			reason = g_strdup_printf(_("%s is not a valid group."), group_name);
		}
	}
	else
	{
		reason = g_strdup(_("Unknown error."));
	}

	title = g_strdup_printf(_("%s on %s (%s)"), msg,
						  purple_account_get_username(account),
						  purple_account_get_protocol_name(account));
	purple_notify_error(gc, NULL, title, reason);
	g_free(title);
	g_free(reason);
}

/**************************************************************************
 * Login
 **************************************************************************/

static void rcon_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	connect_cb(cmdproc->servconn);
}

//void
//nateon_got_login_params(NateonSession *session, const char *login_params)
//{
//	NateonCmdProc *cmdproc;
//
//	cmdproc = session->notification->cmdproc;
//
//	nateon_session_set_login_step(session, NATEON_LOGIN_STEP_AUTH_END);
//
//	nateon_cmdproc_send(cmdproc, "USR", "TWN S %s", login_params);
//}

//static void
//usr_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session;
//	PurpleAccount *account;
//	PurpleConnection *gc;
//
//	session = cmdproc->session;
//	account = session->account;
//	gc = purple_account_get_connection(account);
//
//	if (!g_ascii_strcasecmp(cmd->params[1], "OK"))
//	{
//		/* OK */
//		const char *friendly = purple_url_decode(cmd->params[3]);
//
//		purple_connection_set_display_name(gc, friendly);
//
//		nateon_session_set_login_step(session, NATEON_LOGIN_STEP_SYN);
//
//		nateon_cmdproc_send(cmdproc, "SYN", "%s", "0");
//	}
//	else if (!g_ascii_strcasecmp(cmd->params[1], "TWN"))
//	{
//		/* Passport authentication */
//		char **elems, **cur, **tokens;
//
//		session->nexus = nateon_nexus_new(session);
//
//		/* Parse the challenge data. */
//
//		elems = g_strsplit(cmd->params[3], ",", 0);
//
//		for (cur = elems; *cur != NULL; cur++)
//		{
//				tokens = g_strsplit(*cur, "=", 2);
//				g_hash_table_insert(session->nexus->challenge_data, tokens[0], tokens[1]);
//				/* Don't free each of the tokens, only the array. */
//				g_free(tokens);
//		}
//
//		g_strfreev(elems);
//
//		nateon_session_set_login_step(session, NATEON_LOGIN_STEP_AUTH_START);
//
//		nateon_nexus_connect(session->nexus);
//	}
//}

//static void error_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonErrorType nateonerr = 0;
//
//	switch (atoi(cmd->command))
//	{
//		case 301:
//			nateonerr = NATEON_ERROR_AUTH;
//			break;
//		default:
//			return;
//			break;
//	}
//	nateon_session_set_error(cmdproc->session, nateonerr, NULL);
//}

static void lsin_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
{
	NateonErrorType nateonerr = 0;

	switch (error)
	{
//		case 500:
//		case 601:
//		case 910:
//		case 921:
//			nateonerr = NATEON_ERROR_SERV_UNAVAILABLE;
//			break;
//		case 911:
		case 301:
			nateonerr = NATEON_ERROR_AUTH;
			break;
		default:
			return;
			break;
	}

	nateon_session_set_error(cmdproc->session, nateonerr, NULL);
}

static void pver_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	nateon_cmdproc_send(cmdproc, "AUTH", "DES");
}

static void auth_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	PurpleAccount *account;

	account = cmdproc->session->account;

	nateon_cmdproc_send(cmdproc, "REQS", "DES %s", purple_account_get_username(account));
}

static void reqs_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	char *host;
	int port;

	session = cmdproc->session;

	host = g_strdup(cmd->params[2]);
	port = atoi(cmd->params[3]);

	nateon_session_set_login_step(session, NATEON_LOGIN_STEP_TRANSFER);
	nateon_notification_connect(session->notification, host, port);

	g_free(host);
}

static void lsin_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	PurpleConnection *gc;
	NateonSync *sync;
	NateonUser *user;
	char *user_id, *stored, *friend;

	session = cmdproc->session;
	gc = purple_account_get_connection(session->account);

	nateon_session_set_login_step(session, NATEON_LOGIN_STEP_AUTH);

	sync = nateon_sync_new(session);
	sync->old_cbs_table = cmdproc->cbs_table;

	session->sync = sync;
	cmdproc->cbs_table = sync->cbs_table;

	user_id = cmd->params[1];
	stored  = purple_strreplace(cmd->params[3], "%20", " "); // 별칭
	friend  = cmd->params[2]; // 본명

	user = session->user;

	g_free(user->id);
	user->id = g_strdup(user_id);

	g_free(user->store_name);
	if (!g_strncasecmp(stored, "%00", 3))
	{
		user->store_name = g_strdup(friend);
	}
	else
	{
		user->store_name = g_strdup(stored);
	}

	g_free(user->friendly_name);
	user->friendly_name = g_strdup(friend);

	if (session->ticket != NULL)
		g_free(session->ticket);
	session->ticket = g_strdup(cmd->params[6]);

	purple_connection_set_display_name(gc, user->store_name);

	//nateon_cmdproc_send(cmdproc, "CONF", "0 0");
	nateon_cmdproc_send(cmdproc, "GLST", NULL);
}

static void infy_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session = cmdproc->session;
	NateonUser *user;

	user = nateon_userlist_find_user_with_name(session->userlist, cmd->params[1]);

	nateon_user_set_state(user, cmd->params[2]);
	nateon_user_update(user);
}

static void ntfy_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session = cmdproc->session;
	NateonUser *user;

	user = nateon_userlist_find_user_with_name(session->userlist, cmd->params[1]);

	nateon_user_set_state(user, cmd->params[2]);
	nateon_user_update(user);
}

static void ping_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	nateon_cmdproc_send(cmdproc, "PING", NULL);
}

/**************************************************************************
 * Log out
 **************************************************************************/

static void
kill_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
//	if (!g_ascii_strcasecmp(cmd->params[0], "OTH"))
		nateon_session_set_error(cmdproc->session, NATEON_ERROR_SIGN_OTHER,
							  NULL);
//	else if (!g_ascii_strcasecmp(cmd->params[0], "SSD"))
//		nateon_session_set_error(cmdproc->session, NATEON_ERROR_SERV_DOWN, NULL);
}

void
nateon_notification_close(NateonNotification *notification)
{
	g_return_if_fail(notification != NULL);

	if (!notification->in_use)
		return;

//	nateon_cmdproc_send_quick(notification->cmdproc, "OUT", NULL, NULL);

	nateon_notification_disconnect(notification);
}

/**************************************************************************
 * CTOC: Clent to Clent 
 **************************************************************************/

static void
ctoc_cmd_post(NateonCmdProc *cmdproc, NateonCommand *cmd, char *payload,
			 size_t len)
{
	char *cmd_string;
	NateonCommand *new_cmd;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
	purple_debug_info("nateon", "[%s]\n%s\n", __FUNCTION__, payload);

	cmd_string = purple_strreplace(payload, "\r\n", " ");
	new_cmd = nateon_command_from_string(cmd_string);

	//nateon_cmdproc_process_cmd_text(cmdproc, command);
	nateon_cmdproc_process_cmd(cmdproc, new_cmd);

	g_free(cmd_string);
	nateon_command_unref(new_cmd);
}

static void
ctoc_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	cmdproc->last_cmd->payload_cb  = ctoc_cmd_post;
	cmdproc->servconn->payload_len = atoi(cmd->params[2]);
}

///**************************************************************************
// * Messages
// **************************************************************************/
//
//static void
//msg_cmd_post(NateonCmdProc *cmdproc, NateonCommand *cmd, char *payload,
//			 size_t len)
//{
//	NateonMessage *msg;
//
//	msg = nateon_message_new_from_cmd(cmdproc->session, cmd);
//
//	nateon_message_parse_payload(msg, payload, len);
//#ifdef NATEON_DEBUG_NS
//	nateon_message_show_readable(msg, "Notification", TRUE);
//#endif
//
//	nateon_cmdproc_process_msg(cmdproc, msg);
//
//	nateon_message_destroy(msg);
//}
//
//static void
//msg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	/* NOTE: cmd is not always cmdproc->last_cmd, sometimes cmd is a queued
//	 * command and we are processing it */
//
//	if (cmd->payload == NULL)
//	{
//		cmdproc->last_cmd->payload_cb  = msg_cmd_post;
//		cmdproc->servconn->payload_len = atoi(cmd->params[2]);
//	}
//	else
//	{
//		g_return_if_fail(cmd->payload_cb != NULL);
//
//		cmd->payload_cb(cmdproc, cmd, cmd->payload, cmd->payload_len);
//	}
//}
//
///**************************************************************************
// * Challenges
// **************************************************************************/
//
//static void
//chl_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonTransaction *trans;
//	char buf[33];
//	const char *challenge_resp;
//	PurpleCipher *cipher;
//	PurpleCipherContext *context;
//	guchar digest[16];
//	int i;
//
//	cipher = purple_ciphers_find_cipher("md5");
//	context = purple_cipher_context_new(cipher, NULL);
//
//	purple_cipher_context_append(context, (const guchar *)cmd->params[1],
//							   strlen(cmd->params[1]));
//
//	challenge_resp = "VT6PX?UQTM4WM%YR";
//
//	purple_cipher_context_append(context, (const guchar *)challenge_resp,
//							   strlen(challenge_resp));
//	purple_cipher_context_digest(context, sizeof(digest), digest, NULL);
//	purple_cipher_context_destroy(context);
//
//	for (i = 0; i < 16; i++)
//		g_snprintf(buf + (i*2), 3, "%02x", digest[i]);
//
//	trans = nateon_transaction_new(cmdproc, "QRY", "%s 32", "PROD0038W!61ZTF9");
//
//	nateon_transaction_set_payload(trans, buf, 32);
//
//	nateon_cmdproc_send_trans(cmdproc, trans);
//}

/**************************************************************************
 * Buddy Lists
 **************************************************************************/

static void
adsb_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	NateonUser *user;
	char **params;
	const char *action;
	const char *account;
	const char *user_id;
	int list_op;
	NateonListId list_id;
	int group_id;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	params   = g_strsplit(cmd->trans->params, " ", 0);
	action   = cmd->params[1];
	account  = params[2];
	user_id  = cmd->params[1];
	group_id = atoi(params[3]);

	session = cmdproc->session;

	user = nateon_userlist_find_user_with_name(session->userlist, account);

	if (user == NULL)
	{
		purple_debug_info("nateon", "user == NULL\n");
		user = nateon_user_new(session->userlist, account, "", user_id);
		nateon_userlist_add_user(session->userlist, user);
	}
//	else
//		nateon_user_set_friendly_name(user, friendly);

//	list_id = nateon_get_list_id(list);

//	if (cmd->param_count >= 6)
//		group_id = atoi(cmd->params[5]);
//	else
//		group_id = 0;

	nateon_got_add_user(session, user, NATEON_LIST_FL, group_id);
	nateon_user_update(user);
}

static void
addb_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	NateonUser *user;
	char *user_id;
	char *account;
	char *list;
	int list_id;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	session = cmdproc->session;


	/* Sync */
	if (cmd->trans)
	{
		char **params;

		params = g_strsplit(cmd->trans->params, " ", 0);

		list = params[0];
		user_id = cmd->params[1];
		account = params[2];
	}	
	/* Async*/
	else
	{
		list = cmd->params[1];
		user_id = cmd->params[2];
		account = cmd->params[3];
	}
	purple_debug_info("nateon", "[%s] list(%s), user_id(%s), account(%s)\n", __FUNCTION__, list, user_id, account);

	user = nateon_userlist_find_user_with_id(session->userlist, user_id);
        if (user == NULL)
        {
                user = nateon_user_new(session->userlist, account, "", user_id);
                user->group_ids = NULL;
                nateon_userlist_add_user(session->userlist, user);
        }

	list_id = nateon_get_list_id(list);
	
	nateon_got_add_user(session, user, list_id, -1);
	nateon_user_update(user);
}

//static void
//add_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
//{
//	NateonSession *session;
//	PurpleAccount *account;
//	PurpleConnection *gc;
//	const char *list, *passport;
//	char *reason = NULL;
//	char *msg = NULL;
//	char **params;
//
//	session = cmdproc->session;
//	account = session->account;
//	gc = purple_account_get_connection(account);
//	params = g_strsplit(trans->params, " ", 0);
//
//	list     = params[0];
//	passport = params[1];
//
//	if (!strcmp(list, "FL"))
//		msg = g_strdup_printf(_("Unable to add user on %s (%s)"),
//							  purple_account_get_username(account),
//							  purple_account_get_protocol_name(account));
//	else if (!strcmp(list, "BL"))
//		msg = g_strdup_printf(_("Unable to block user on %s (%s)"),
//							  purple_account_get_username(account),
//							  purple_account_get_protocol_name(account));
//	else if (!strcmp(list, "AL"))
//		msg = g_strdup_printf(_("Unable to permit user on %s (%s)"),
//							  purple_account_get_username(account),
//							  purple_account_get_protocol_name(account));
//
//	if (!strcmp(list, "FL"))
//	{
//		if (error == 210)
//		{
//			reason = g_strdup_printf(_("%s could not be added because "
//									   "your buddy list is full."), passport);
//		}
//	}
//
//	if (reason == NULL)
//	{
//		if (error == 208)
//		{
//			reason = g_strdup_printf(_("%s is not a valid passport account."),
//									 passport);
//		}
//		else if (error == 500)
//		{
//			reason = g_strdup(_("Service Temporarily Unavailable."));
//		}
//		else
//		{
//			reason = g_strdup(_("Unknown error."));
//		}
//	}
//
//	if (msg != NULL)
//	{
//		purple_notify_error(gc, NULL, msg, reason);
//		g_free(msg);
//	}
//
//	if (!strcmp(list, "FL"))
//	{
//		PurpleBuddy *buddy;
//
//		buddy = purple_find_buddy(account, passport);
//
//		if (buddy != NULL)
//			purple_blist_remove_buddy(buddy);
//	}
//
//	g_free(reason);
//
//	g_strfreev(params);
//}

static void
addg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonGroup *group;
	NateonSession *session;
	gint group_id;
	char **params;
	const char *group_name;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	session = cmdproc->session;

	group_id = atoi(cmd->params[2]);

	params = g_strsplit(cmd->trans->params, " ", 0);
	group_name = purple_strreplace(params[1], "%20", " ");

	group = nateon_group_new(session->userlist, group_id, group_name);

	/* There is a user that must me moved to this group */
	if (cmd->trans->data)
	{
		/* nateon_userlist_move_buddy(); */
		NateonUserList *userlist = cmdproc->session->userlist;
		NateonMoveBuddy *data = cmd->trans->data;

		if (data->old_group_name != NULL)
		{
			nateon_userlist_move_buddy(userlist, data->who, data->old_group_name, group_name);
			g_free(data->old_group_name);
		}
		else
		{
			nateon_userlist_add_buddy(userlist, data->who, NATEON_LIST_FL, group_name);
		}

		g_free(data->who);

	}
}

//static void
//fln_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSlpLink *slplink;
//	NateonUser *user;
//
//	user = nateon_userlist_find_user(cmdproc->session->userlist, cmd->params[0]);
//
//	user->status = "offline";
//	nateon_user_update(user);
//
//	slplink = nateon_session_find_slplink(cmdproc->session, cmd->params[0]);
//
//	if (slplink != NULL)
//		nateon_slplink_destroy(slplink);
//
//}
//
//static void
//iln_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session;
//	PurpleAccount *account;
//	PurpleConnection *gc;
//	NateonUser *user;
//	NateonObject *nateonobj;
//	const char *state, *passport, *friendly;
//
//	session = cmdproc->session;
//	account = session->account;
//	gc = purple_account_get_connection(account);
//
//	state    = cmd->params[1];
//	passport = cmd->params[2];
//	friendly = purple_url_decode(cmd->params[3]);
//
//	user = nateon_userlist_find_user(session->userlist, passport);
//
//	serv_got_alias(gc, passport, friendly);
//
//	nateon_user_set_friendly_name(user, friendly);
//
//	if (session->protocol_ver >= 9 && cmd->param_count == 6)
//	{
//		nateonobj = nateon_object_new_from_string(purple_url_decode(cmd->params[5]));
//		nateon_user_set_object(user, nateonobj);
//	}
//
//	nateon_user_set_state(user, state);
//	nateon_user_update(user);
//}
//
//static void
//ipg_cmd_post(NateonCmdProc *cmdproc, NateonCommand *cmd, char *payload, size_t len)
//{
//#if 0
//	purple_debug_misc("nateon", "Incoming Page: {%s}\n", payload);
//#endif
//}
//
//static void
//ipg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	cmdproc->servconn->payload_len = atoi(cmd->params[0]);
//	cmdproc->last_cmd->payload_cb = ipg_cmd_post;
//}

static void
nprf_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	purple_debug_info("nateon", "[%s] what is this? @,.@\n", __FUNCTION__);
}

static void
nnik_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
        PurpleAccount *account;
        PurpleConnection *gc;
        NateonUser *user;
        const char *store_name;

        account = cmdproc->session->account;
        gc = purple_account_get_connection(account);

        //store_name = purple_url_decode(cmd->params[2]);
        if (!g_strncasecmp(cmd->params[2], "%00", 3))
	{
   		store_name = NULL;
	}
	else
	{
    		store_name = purple_url_decode(cmd->params[2]);
	}

	user = nateon_userlist_find_user_with_name(cmdproc->session->userlist, cmd->params[1]);

        nateon_user_set_store_name(user, store_name);

        nateon_user_set_buddy_alias(gc->proto_data, user);

        nateon_user_update(user);
}

//static void
//nln_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session;
//	PurpleAccount *account;
//	PurpleConnection *gc;
//	NateonUser *user;
//	NateonObject *nateonobj;
//	int clientid;
//	const char *state, *passport, *friendly;
//
//	session = cmdproc->session;
//	account = session->account;
//	gc = purple_account_get_connection(account);
//
//	state    = cmd->params[0];
//	passport = cmd->params[1];
//	friendly = purple_url_decode(cmd->params[2]);
//
//	user = nateon_userlist_find_user(session->userlist, passport);
//
//	serv_got_alias(gc, passport, friendly);
//
//	nateon_user_set_friendly_name(user, friendly);
//
//	if (session->protocol_ver >= 9)
//	{
//		if (cmd->param_count == 5)
//		{
//			nateonobj =
//				nateon_object_new_from_string(purple_url_decode(cmd->params[4]));
//			nateon_user_set_object(user, nateonobj);
//		}
//		else
//		{
//			nateon_user_set_object(user, NULL);
//		}
//	}
//
//	clientid = atoi(cmd->params[3]);
//	user->mobile = (clientid & NATEON_CLIENT_CAP_NATEONMOBILE);
//
//	nateon_user_set_state(user, state);
//	nateon_user_update(user);
//}
//
//#if 0
//static void
//chg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	char *state = cmd->params[1];
//	int state_id = 0;
//
//	if (!strcmp(state, "NLN"))
//		state_id = NATEON_ONLINE;
//	else if (!strcmp(state, "BSY"))
//		state_id = NATEON_BUSY;
//	else if (!strcmp(state, "IDL"))
//		state_id = NATEON_IDLE;
//	else if (!strcmp(state, "BRB"))
//		state_id = NATEON_BRB;
//	else if (!strcmp(state, "AWY"))
//		state_id = NATEON_AWAY;
//	else if (!strcmp(state, "PHN"))
//		state_id = NATEON_PHONE;
//	else if (!strcmp(state, "LUN"))
//		state_id = NATEON_LUNCH;
//	else if (!strcmp(state, "HDN"))
//		state_id = NATEON_HIDDEN;
//
//	cmdproc->session->state = state_id;
//}
//#endif
//
//
//static void
//not_cmd_post(NateonCmdProc *cmdproc, NateonCommand *cmd, char *payload, size_t len)
//{
//#if 0
//	NATEON_SET_PARAMS("NOT %d\r\n%s", cmdproc->servconn->payload, payload);
//	purple_debug_misc("nateon", "Notification: {%s}\n", payload);
//#endif
//}
//
//static void
//not_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	cmdproc->servconn->payload_len = atoi(cmd->params[0]);
//	cmdproc->last_cmd->payload_cb = not_cmd_post;
//}
//
//static void
//rea_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	/* TODO: This might be for us too */
//
//	NateonSession *session;
//	PurpleConnection *gc;
//	const char *friendly;
//
//	session = cmdproc->session;
//	gc = session->account->gc;
//	friendly = purple_url_decode(cmd->params[3]);
//
//	purple_connection_set_display_name(gc, friendly);
//}
//
//static void
//prp_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session = cmdproc->session;
//	const char *type, *value;
//
//	g_return_if_fail(cmd->param_count >= 3);
//
//	type  = cmd->params[2];
//
//	if (cmd->param_count == 4)
//	{
//		value = cmd->params[3];
//		if (!strcmp(type, "PHH"))
//			nateon_user_set_home_phone(session->user, purple_url_decode(value));
//		else if (!strcmp(type, "PHW"))
//			nateon_user_set_work_phone(session->user, purple_url_decode(value));
//		else if (!strcmp(type, "PHM"))
//			nateon_user_set_mobile_phone(session->user, purple_url_decode(value));
//	}
//	else
//	{
//		if (!strcmp(type, "PHH"))
//			nateon_user_set_home_phone(session->user, NULL);
//		else if (!strcmp(type, "PHW"))
//			nateon_user_set_work_phone(session->user, NULL);
//		else if (!strcmp(type, "PHM"))
//			nateon_user_set_mobile_phone(session->user, NULL);
//	}
//}

static void
reng_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	char **params;
	int group_id;
	const char *group_name;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	session = cmdproc->session;
	params = g_strsplit(cmd->trans->params, " ", 0);
	group_id = atoi(params[1]);
	group_name = purple_strreplace(params[2], "%20", " ");

	nateon_userlist_rename_group_id(session->userlist, group_id, group_name);
}

static void
reng_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
{
	int group_id;
	char **params;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	params = g_strsplit(trans->params, " ", 0);
	group_id = atoi(params[1]);

	group_error_helper(cmdproc->session, _("Unable to rename group"), group_id, error);

	g_strfreev(params);
}

static void
rmvb_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	NateonUser *user;
	const char *list;
	const char *account;
	int group_id;
	NateonListId list_id;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	session = cmdproc->session;

	/* Sync */
	if (cmd->trans) {
		char **params;

		purple_debug_info("nateon", "[%s] Sync\n", __FUNCTION__);

		params = g_strsplit(cmd->trans->params, " ", 0);

		list = params[0];
		account = params[2];
		group_id = atoi(params[3]);

		purple_debug_info("nateon", "[%s] account(%s), list(%s), group_id(%d)\n", __FUNCTION__, account, list, group_id);
	}
	/* Async */
	else {
		purple_debug_info("nateon", "[%s] Async\n", __FUNCTION__);

		list = cmd->params[1];
		account = cmd->params[3];
		group_id = -1;

		purple_debug_info("nateon", "[%s] account(%s), list(%s), group_id(%d)\n", __FUNCTION__, account, list, group_id);
	}
	user = nateon_userlist_find_user_with_name(session->userlist, account);

	g_return_if_fail(user != NULL);

	list_id = nateon_get_list_id(list);

	nateon_got_rem_user(session, user, list_id, group_id);
	nateon_user_update(user);
}

static void
rmvg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	char **params;
	int group_id;

	params = g_strsplit(cmd->trans->params, " ", 0);
	session = cmdproc->session;
	group_id = atoi(params[1]);

	nateon_userlist_remove_group_id(session->userlist, group_id);
}

static void
mvbg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	NateonUser *user;
	char **params;
	const char *account;
	int old_group_id;
	int new_group_id;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	params   = g_strsplit(cmd->trans->params, " ", 0);
	account  = params[3];
	old_group_id = atoi(params[4]);
	new_group_id = atoi(params[5]);

	purple_debug_info("nateon", "[%s] account(%s), old(%d), new(%d)\n", __FUNCTION__, account, old_group_id, new_group_id);

	session = cmdproc->session;

	user = nateon_userlist_find_user_with_name(session->userlist, account);

	nateon_user_add_group_id(user, new_group_id);
	nateon_user_remove_group_id(user, old_group_id);
	nateon_user_update(user);
}

//static void
//rmg_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
//{
//	int group_id;
//	char **params;
//
//	params = g_strsplit(trans->params, " ", 0);
//
//	group_id = atoi(params[0]);
//
//	group_error_helper(cmdproc->session, _("Unable to delete group"), group_id, error);
//
//	g_strfreev(params);
//}
//
//static void
//syn_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session;
//	int total_users;
//
//	session = cmdproc->session;
//
//	if (cmd->param_count == 2)
//	{
//		/*
//		 * This can happen if we sent a SYN with an up-to-date
//		 * buddy list revision, but we send 0 to get a full list.
//		 * So, error out.
//		 */
//
//		nateon_session_set_error(cmdproc->session, NATEON_ERROR_BAD_BLIST, NULL);
//		return;
//	}
//
//	total_users  = atoi(cmd->params[2]);
//
//	if (total_users == 0)
//	{
//		nateon_session_finish_login(session);
//	}
//	else
//	{
//		/* syn_table */
//		NateonSync *sync;
//
//		sync = nateon_sync_new(session);
//		sync->total_users = total_users;
//		sync->old_cbs_table = cmdproc->cbs_table;
//
//		session->sync = sync;
//		cmdproc->cbs_table = sync->cbs_table;
//	}
//}

/**************************************************************************
 * Misc commands
 **************************************************************************/

//static void
//url_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session;
//	PurpleAccount *account;
//	const char *rru;
//	const char *url;
//	PurpleCipher *cipher;
//	PurpleCipherContext *context;
//	guchar digest[16];
//	FILE *fd;
//	char *buf;
//	char buf2[3];
//	char sendbuf[64];
//	int i;
//
//	session = cmdproc->session;
//	account = session->account;
//
//	rru = cmd->params[1];
//	url = cmd->params[2];
//
//	buf = g_strdup_printf("%s%lu%s",
//			   session->passport_info.mspauth,
//			   time(NULL) - session->passport_info.sl,
//			   purple_connection_get_password(account->gc));
//
//	cipher = purple_ciphers_find_cipher("md5");
//	context = purple_cipher_context_new(cipher, NULL);
//
//	purple_cipher_context_append(context, (const guchar *)buf, strlen(buf));
//	purple_cipher_context_digest(context, sizeof(digest), digest, NULL);
//	purple_cipher_context_destroy(context);
//
//	g_free(buf);
//
//	memset(sendbuf, 0, sizeof(sendbuf));
//
//	for (i = 0; i < 16; i++)
//	{
//		g_snprintf(buf2, sizeof(buf2), "%02x", digest[i]);
//		strcat(sendbuf, buf2);
//	}
//
//	if (session->passport_info.file != NULL)
//	{
//		g_unlink(session->passport_info.file);
//		g_free(session->passport_info.file);
//	}
//
//	if ((fd = purple_mkstemp(&session->passport_info.file, FALSE)) == NULL)
//	{
//		purple_debug_error("nateon",
//						 "Error opening temp passport file: %s\n",
//						 strerror(errno));
//	}
//	else
//	{
//		fputs("<html>\n"
//			  "<head>\n"
//			  "<noscript>\n"
//			  "<meta http-equiv=\"Refresh\" content=\"0; "
//			  "url=http://www.hotmail.com\">\n"
//			  "</noscript>\n"
//			  "</head>\n\n",
//			  fd);
//
//		fprintf(fd, "<body onload=\"document.pform.submit(); \">\n");
//		fprintf(fd, "<form name=\"pform\" action=\"%s\" method=\"POST\">\n\n",
//				url);
//		fprintf(fd, "<input type=\"hidden\" name=\"mode\" value=\"ttl\">\n");
//		fprintf(fd, "<input type=\"hidden\" name=\"login\" value=\"%s\">\n",
//				purple_account_get_username(account));
//		fprintf(fd, "<input type=\"hidden\" name=\"username\" value=\"%s\">\n",
//				purple_account_get_username(account));
//		fprintf(fd, "<input type=\"hidden\" name=\"sid\" value=\"%s\">\n",
//				session->passport_info.sid);
//		fprintf(fd, "<input type=\"hidden\" name=\"kv\" value=\"%s\">\n",
//				session->passport_info.kv);
//		fprintf(fd, "<input type=\"hidden\" name=\"id\" value=\"2\">\n");
//		fprintf(fd, "<input type=\"hidden\" name=\"sl\" value=\"%ld\">\n",
//				time(NULL) - session->passport_info.sl);
//		fprintf(fd, "<input type=\"hidden\" name=\"rru\" value=\"%s\">\n",
//				rru);
//		fprintf(fd, "<input type=\"hidden\" name=\"auth\" value=\"%s\">\n",
//				session->passport_info.mspauth);
//		fprintf(fd, "<input type=\"hidden\" name=\"creds\" value=\"%s\">\n",
//				sendbuf); /* TODO Digest me (huh? -- ChipX86) */
//		fprintf(fd, "<input type=\"hidden\" name=\"svc\" value=\"mail\">\n");
//		fprintf(fd, "<input type=\"hidden\" name=\"js\" value=\"yes\">\n");
//		fprintf(fd, "</form></body>\n");
//		fprintf(fd, "</html>\n");
//
//		if (fclose(fd))
//		{
//			purple_debug_error("nateon",
//							 "Error closing temp passport file: %s\n",
//							 strerror(errno));
//
//			g_unlink(session->passport_info.file);
//			g_free(session->passport_info.file);
//			session->passport_info.file = NULL;
//		}
//#ifdef _WIN32
//		else
//		{
//			/*
//			 * Renaming file with .html extension, so that the
//			 * win32 open_url will work.
//			 */
//			char *tmp;
//
//			if ((tmp =
//				g_strdup_printf("%s.html",
//					session->passport_info.file)) != NULL)
//			{
//				if (g_rename(session->passport_info.file,
//							tmp) == 0)
//				{
//					g_free(session->passport_info.file);
//					session->passport_info.file = tmp;
//				}
//				else
//					g_free(tmp);
//			}
//		}
//#endif
//	}
//}

static void
cnik_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	PurpleConnection *gc;
	char *friendly;

	gc = purple_account_get_connection(cmdproc->session->account);

	friendly = purple_strreplace(cmd->trans->params, "%20", " ");
	purple_connection_set_display_name(gc, friendly);
}

static void tick_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{	
	NateonSession *session;

	session = cmdproc->session;

	if (session->ticket != NULL)
		g_free(session->ticket);
	session->ticket = g_strdup(cmd->params[1]);
}

static void
imsg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	PurpleConnection *gc;
	PurpleRequestFields *fields;
	PurpleRequestFieldGroup *g;
	PurpleRequestField *f;
	const char *buddy_name;
	char *contents;
	char *date;
	int i;

	gc = purple_account_get_connection(cmdproc->session->account);

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
	purple_debug_info("nateon", "[%s] param_count(%d)\n", __FUNCTION__, cmd->param_count);

	for (i = 0; i < (cmd->param_count - 1); i++)
	{
		char **params;

		purple_debug_info("nateon", "%d: [%s]\n", i, cmd->params[i]);

		params = g_strsplit(cmd->params[i], ":", 0);
 
        if (params[0] == NULL || !strcmp(params[0], "uuid"))
        {

            params = &cmd->params[i+1];
            contents = g_strjoinv(" ", params);
            contents = purple_strreplace(contents, "\n", "<br>");
            g_strstrip(contents);
            break;
        }
        else if (!strcmp(params[0], "from"))
		{
			buddy_name = params[1];
		}
		else if (!strcmp(params[0], "date"))
		{
			int year, mon, day, hour, min, sec;
			sscanf(params[1], "%04d%02d%02d%02d%02d%02d", &year, &mon, &day, &hour, &min, &sec);
			date = g_strdup_printf("%04d-%02d-%2d %02d:%02d:%02d", year, mon, day, hour, min, sec);
		}
	}

	purple_debug_info("nateon", "[%s] contnets(%s)\n", __FUNCTION__, contents);

	purple_notify_formatted(gc, "쪽지", buddy_name, date, contents, NULL, gc);
}

/**************************************************************************
 * Switchboards
 **************************************************************************/

static void
invt_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	NateonSwitchBoard *swboard;
	const char *session_id;
	char *host;
	int port;

	session = cmdproc->session;
	session_id = cmd->params[3];

	host = g_strdup(cmd->params[1]);
	port = atoi(cmd->params[2]);

//	if (session->http_method)
//		port = 80;


	swboard = nateon_switchboard_new(session);

	nateon_switchboard_set_invited(swboard, TRUE);
//	nateon_switchboard_set_session_id(swboard, cmd->params[0]);
	nateon_switchboard_set_auth_key(swboard, cmd->params[3]);
	swboard->im_user = g_strdup(cmd->params[0]);
	/* nateon_switchboard_add_user(swboard, cmd->params[4]); */

	if (!nateon_switchboard_connect(swboard, host, port))
		nateon_switchboard_destroy(swboard);

	g_free(host);
}

static void
reqc_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;

	session = cmdproc->session;

	nateon_xfer_parse_reqc(session, cmdproc, cmd->params, cmd->param_count);
}

static void
refr_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
    NateonSession *session;

    session = cmdproc->session;

    nateon_xfer_parse_refr(session, cmd->params, cmd->param_count);
}

//static void
//xfr_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	char *host;
//	int port;
//
//	if (strcmp(cmd->params[1], "SB") && strcmp(cmd->params[1], "NS"))
//	{
//		/* Maybe we can have a generic bad command error. */
//		purple_debug_error("nateon", "Bad XFR command (%s)\n", cmd->params[1]);
//		return;
//	}
//
//	nateon_parse_socket(cmd->params[2], &host, &port);
//
//	if (!strcmp(cmd->params[1], "SB"))
//	{
//		purple_debug_error("nateon", "This shouldn't be handled here.\n");
//	}
//	else if (!strcmp(cmd->params[1], "NS"))
//	{
//		NateonSession *session;
//
//		session = cmdproc->session;
//
//		nateon_session_set_login_step(session, NATEON_LOGIN_STEP_TRANSFER);
//
//		nateon_notification_connect(session->notification, host, port);
//	}
//
//	g_free(host);
//}

/**************************************************************************
 * Message Types
 **************************************************************************/

//static void
//profile_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	NateonSession *session;
//	const char *value;
//
//	session = cmdproc->session;
//
//	if (strcmp(msg->remote_user, "Hotmail"))
//		/* This isn't an official message. */
//		return;
//
//	if ((value = nateon_message_get_attr(msg, "kv")) != NULL)
//	{
//		if (session->passport_info.kv != NULL)
//			g_free(session->passport_info.kv);
//
//		session->passport_info.kv = g_strdup(value);
//	}
//
//	if ((value = nateon_message_get_attr(msg, "sid")) != NULL)
//	{
//		if (session->passport_info.sid != NULL)
//			g_free(session->passport_info.sid);
//
//		session->passport_info.sid = g_strdup(value);
//	}
//
//	if ((value = nateon_message_get_attr(msg, "MSPAuth")) != NULL)
//	{
//		if (session->passport_info.mspauth != NULL)
//			g_free(session->passport_info.mspauth);
//
//		session->passport_info.mspauth = g_strdup(value);
//	}
//
//	if ((value = nateon_message_get_attr(msg, "ClientIP")) != NULL)
//	{
//		if (session->passport_info.client_ip != NULL)
//			g_free(session->passport_info.client_ip);
//
//		session->passport_info.client_ip = g_strdup(value);
//	}
//
//	if ((value = nateon_message_get_attr(msg, "ClientPort")) != NULL)
//		session->passport_info.client_port = ntohs(atoi(value));
//
//	if ((value = nateon_message_get_attr(msg, "LoginTime")) != NULL)
//		session->passport_info.sl = atol(value);
//}
//
//static void
//initial_email_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	NateonSession *session;
//	PurpleConnection *gc;
//	GHashTable *table;
//	const char *unread;
//
//	session = cmdproc->session;
//	gc = session->account->gc;
//
//	if (strcmp(msg->remote_user, "Hotmail"))
//		/* This isn't an official message. */
//		return;
//
//	if (session->passport_info.file == NULL)
//	{
//		NateonTransaction *trans;
//		trans = nateon_transaction_new(cmdproc, "URL", "%s", "INBOX");
//		nateon_transaction_queue_cmd(trans, msg->cmd);
//
//		nateon_cmdproc_send_trans(cmdproc, trans);
//
//		return;
//	}
//
//	if (!purple_account_get_check_mail(session->account))
//		return;
//
//	table = nateon_message_get_hashtable_from_body(msg);
//
//	unread = g_hash_table_lookup(table, "Inbox-Unread");
//
//	if (unread != NULL)
//	{
//		int count = atoi(unread);
//
//		if (count > 0)
//		{
//			const char *passport;
//			const char *url;
//
//			passport = nateon_user_get_passport(session->user);
//			url = session->passport_info.file;
//
//			purple_notify_emails(gc, atoi(unread), FALSE, NULL, NULL,
//							   &passport, &url, NULL, NULL);
//		}
//	}
//
//	g_hash_table_destroy(table);
//}
//
//static void
//email_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	NateonSession *session;
//	PurpleConnection *gc;
//	GHashTable *table;
//	char *from, *subject, *tmp;
//
//	session = cmdproc->session;
//	gc = session->account->gc;
//
//	if (strcmp(msg->remote_user, "Hotmail"))
//		/* This isn't an official message. */
//		return;
//
//	if (session->passport_info.file == NULL)
//	{
//		NateonTransaction *trans;
//		trans = nateon_transaction_new(cmdproc, "URL", "%s", "INBOX");
//		nateon_transaction_queue_cmd(trans, msg->cmd);
//
//		nateon_cmdproc_send_trans(cmdproc, trans);
//
//		return;
//	}
//
//	if (!purple_account_get_check_mail(session->account))
//		return;
//
//	table = nateon_message_get_hashtable_from_body(msg);
//
//	from = subject = NULL;
//
//	tmp = g_hash_table_lookup(table, "From");
//	if (tmp != NULL)
//		from = purple_mime_decode_field(tmp);
//
//	tmp = g_hash_table_lookup(table, "Subject");
//	if (tmp != NULL)
//		subject = purple_mime_decode_field(tmp);
//
//	purple_notify_email(gc,
//					  (subject != NULL ? subject : ""),
//					  (from != NULL ?  from : ""),
//					  nateon_user_get_passport(session->user),
//					  session->passport_info.file, NULL, NULL);
//
//	if (from != NULL)
//		g_free(from);
//
//	if (subject != NULL)
//		g_free(subject);
//
//	g_hash_table_destroy(table);
//}
//
//static void
//system_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	GHashTable *table;
//	const char *type_s;
//
//	if (strcmp(msg->remote_user, "Hotmail"))
//		/* This isn't an official message. */
//		return;
//
//	table = nateon_message_get_hashtable_from_body(msg);
//
//	if ((type_s = g_hash_table_lookup(table, "Type")) != NULL)
//	{
//		int type = atoi(type_s);
//		char buf[NATEON_BUF_LEN];
//		int minutes;
//
//		switch (type)
//		{
//			case 1:
//				minutes = atoi(g_hash_table_lookup(table, "Arg1"));
//				g_snprintf(buf, sizeof(buf), ngettext(
//							"The NATEON server will shut down for maintenance "
//							"in %d minute. You will automatically be "
//							"signed out at that time.  Please finish any "
//							"conversations in progress.\n\nAfter the "
//							"maintenance has been completed, you will be "
//							"able to successfully sign in.",
//							"The NATEON server will shut down for maintenance "
//							"in %d minutes. You will automatically be "
//							"signed out at that time.  Please finish any "
//							"conversations in progress.\n\nAfter the "
//							"maintenance has been completed, you will be "
//							"able to successfully sign in.", minutes),
//						minutes);
//			default:
//				break;
//		}
//
//		if (*buf != '\0')
//			purple_notify_info(cmdproc->session->account->gc, NULL, buf, NULL);
//	}
//
//	g_hash_table_destroy(table);
//}

void
nateon_notification_add_buddy(NateonNotification *notification, const char *list,
						   const char *who, const char *user_id,
						   int group_id)
{
	NateonCmdProc *cmdproc;
	cmdproc = notification->servconn->cmdproc;

	purple_debug_info("nateon", "[%s] group_id(%d), list(%s)\n", __FUNCTION__, group_id, strcmp(list,"FL")?"":"FL");

	if (!strcmp(list, "FL"))
	{
		if (group_id < 0)
			group_id = 0;

		nateon_cmdproc_send(cmdproc, "ADSB", "REQST %%00 %s %d", who, group_id);
	}
	else
	{
		nateon_cmdproc_send(cmdproc, "ADDB", "%s %s %s", list, user_id, who);
	}
}

void
nateon_notification_rem_buddy(NateonNotification *notification, const char *list, const char *who, int group_id, const char *account)
{
	NateonCmdProc *cmdproc;
	NateonUser *user ;

	cmdproc = notification->servconn->cmdproc;
	user = cmdproc->session->user;

        purple_debug_info("nateon", "%s - %s%s%s%s\n", __FUNCTION__,
                        (user->list_op & NATEON_LIST_FL_OP) ? "FL" : "",
                        (user->list_op & NATEON_LIST_AL_OP) ? "AL" : "",
                        (user->list_op & NATEON_LIST_BL_OP) ? "BL" : "",
                        (user->list_op & NATEON_LIST_RL_OP) ? "RL" : "");

	nateon_cmdproc_send(cmdproc, "RMVB", "%s %s %s %d", list, account, who, group_id);
//	if (group_id >= 0)
//	{
//		// ToDo: ListId check
//		nateon_cmdproc_send(cmdproc, "REM", "%s %s %d", list, who, group_id);
//	}
//	else
//	{
//		nateon_cmdproc_send(cmdproc, "REM", "%s %s", list, who);
//	}
}

void nateon_notification_move_buddy(NateonNotification *notification, const char *who, const char *user_id, int old_group_id, int new_group_id)
{
	NateonCmdProc *cmdproc;
	char *cmd;

	cmdproc = notification->servconn->cmdproc;
	cmd = g_strdup_printf("0 %s %s %d %d", user_id, who, old_group_id, new_group_id);

	nateon_cmdproc_send(cmdproc, "MVBG", "0 %d\r\n%s", strlen(cmd)+2, cmd);
}

void nateon_notification_copy_buddy(NateonNotification *notification, const char *who, const char *user_id, int old_group_id, int new_group_id)
{
	NateonCmdProc *cmdproc;
	char *cmd;

	cmdproc = notification->servconn->cmdproc;
	cmd = g_strdup_printf("0 %s %s %d %d", user_id, who, old_group_id, new_group_id);

	nateon_cmdproc_send(cmdproc, "CPBG", "0 %d\r\n%s", strlen(cmd)+2, cmd);
}

/**************************************************************************
 * Init
 **************************************************************************/

void
nateon_notification_init(void)
{
	/* TODO: check prp, blp */

	cbs_table = nateon_table_new();

	/* Synchronous */

	// PRS server
	nateon_table_add_cmd(cbs_table, "RCON", "RCON", rcon_cmd);

	// Login/Dispatch server
	nateon_table_add_cmd(cbs_table, "PVER", "PVER", pver_cmd);
	nateon_table_add_cmd(cbs_table, "AUTH", "AUTH", auth_cmd);
	nateon_table_add_cmd(cbs_table, "REQS", "REQS", reqs_cmd);

	// Notification server
	nateon_table_add_cmd(cbs_table, "LSIN", "LSIN", lsin_cmd);
	nateon_table_add_cmd(cbs_table, "ONST", "ONST", NULL);
	nateon_table_add_cmd(cbs_table, "CNIK", "CNIK", cnik_cmd);

	// Buddy & Group
	nateon_table_add_cmd(cbs_table, "ADSB", "ADSB", adsb_cmd);
	nateon_table_add_cmd(cbs_table, "RMVB", "RMVB", rmvb_cmd);
	nateon_table_add_cmd(cbs_table, "ADDG", "ADDG", addg_cmd);
	nateon_table_add_cmd(cbs_table, "RMVG", "RMVG", rmvg_cmd);
	nateon_table_add_cmd(cbs_table, "RENG", "RENG", reng_cmd);
	nateon_table_add_cmd(cbs_table, "MVBG", "MVBG", mvbg_cmd);
	nateon_table_add_cmd(cbs_table, "ADDB", "ADDB", addb_cmd);
	
//	nateon_table_add_cmd(cbs_table, "CHG", "CHG", NULL);
//	nateon_table_add_cmd(cbs_table, "CHG", "ILN", iln_cmd);
//	nateon_table_add_cmd(cbs_table, "ADD", "ADD", add_cmd);
//	nateon_table_add_cmd(cbs_table, "ADD", "ILN", iln_cmd);
//	nateon_table_add_cmd(cbs_table, "REM", "REM", rem_cmd);
//	nateon_table_add_cmd(cbs_table, "USR", "USR", usr_cmd);
//	nateon_table_add_cmd(cbs_table, "USR", "XFR", xfr_cmd);
//	nateon_table_add_cmd(cbs_table, "SYN", "SYN", syn_cmd);
//	nateon_table_add_cmd(cbs_table, "CVR", "CVR", cvr_cmd);
//	nateon_table_add_cmd(cbs_table, "REA", "REA", rea_cmd);
//	nateon_table_add_cmd(cbs_table, "PRP", "PRP", prp_cmd);
//	/* nateon_table_add_cmd(cbs_table, "BLP", "BLP", blp_cmd); */
//	nateon_table_add_cmd(cbs_table, "BLP", "BLP", NULL);
//	nateon_table_add_cmd(cbs_table, "REG", "REG", reg_cmd);
//	nateon_table_add_cmd(cbs_table, "ADG", "ADG", adg_cmd);
//	nateon_table_add_cmd(cbs_table, "RMG", "RMG", rmg_cmd);
//	nateon_table_add_cmd(cbs_table, "XFR", "XFR", xfr_cmd);

	/* Asynchronous */
	nateon_table_add_cmd(cbs_table, NULL, "INFY", infy_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "NTFY", ntfy_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "PING", ping_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "CTOC", ctoc_cmd);

	// CTOC
	nateon_table_add_cmd(cbs_table, NULL, "IMSG", imsg_cmd);
	// TICK
	nateon_table_add_cmd(cbs_table, NULL, "TICK", tick_cmd);

	// Buddy
//	nateon_table_add_cmd(cbs_table, NULL, "ADDB", addb_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "RMVB", rmvb_cmd);

//	nateon_table_add_cmd(cbs_table, NULL, "IPG", ipg_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "MSG", msg_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "NOT", not_cmd);
//
//	nateon_table_add_cmd(cbs_table, NULL, "CHL", chl_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "REM", rem_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "ADD", add_cmd);
//
//	nateon_table_add_cmd(cbs_table, NULL, "QRY", NULL);
//	nateon_table_add_cmd(cbs_table, NULL, "QNG", NULL);
//	nateon_table_add_cmd(cbs_table, NULL, "FLN", fln_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "NLN", nln_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "ILN", iln_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "NPRF", nprf_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "NNIK", nnik_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "KILL", kill_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "INVT", invt_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "REQC", reqc_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "REFR", refr_cmd);

//
//	nateon_table_add_cmd(cbs_table, NULL, "URL", url_cmd);
//
//	nateon_table_add_cmd(cbs_table, "fallback", "XFR", xfr_cmd);
//
	nateon_table_add_error(cbs_table, "RENG", reng_error);
//	nateon_table_add_error(cbs_table, "ADD", add_error);
//	nateon_table_add_error(cbs_table, "REG", reg_error);
//	nateon_table_add_error(cbs_table, "RMG", rmg_error);
//	/* nateon_table_add_error(cbs_table, "REA", rea_error); */
	nateon_table_add_error(cbs_table, "LSIN", lsin_error);

//	nateon_table_add_msg_type(cbs_table,
//						   "text/x-msmsgsprofile",
//						   profile_msg);
//	nateon_table_add_msg_type(cbs_table,
//						   "text/x-msmsgsinitialemailnotification",
//						   initial_email_msg);
//	nateon_table_add_msg_type(cbs_table,
//						   "text/x-msmsgsemailnotification",
//						   email_msg);
//	nateon_table_add_msg_type(cbs_table,
//						   "application/x-msmsgssystemmessage",
//						   system_msg);
}

void
nateon_notification_end(void)
{
	nateon_table_destroy(cbs_table);
}
