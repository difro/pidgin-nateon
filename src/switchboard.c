/**
 * @file switchboard.c NATEON switchboard functions
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
#include "nateon.h"
//#include "prefs.h"
#include "switchboard.h"
//#include "notification.h"
//#include "utils.h"

//#include "error.h"

static NateonTable *cbs_table;

//static void msg_error_helper(NateonCmdProc *cmdproc, NateonMessage *msg,
//							 NateonMsgErrorType error);

/**************************************************************************
 * Main
 **************************************************************************/

NateonSwitchBoard *
nateon_switchboard_new(NateonSession *session)
{
	NateonSwitchBoard *swboard;
	NateonServConn *servconn;

	g_return_val_if_fail(session != NULL, NULL);

	swboard = g_new0(NateonSwitchBoard, 1);

	swboard->session = session;
	swboard->servconn = servconn = nateon_servconn_new(session, NATEON_SERVCONN_SB);
	swboard->cmdproc = servconn->cmdproc;

	swboard->msg_queue = g_queue_new();
	swboard->empty = TRUE;

	swboard->cmdproc->data = swboard;
	swboard->cmdproc->cbs_table = cbs_table;

	session->switches = g_list_append(session->switches, swboard);

	return swboard;
}

void
nateon_switchboard_destroy(NateonSwitchBoard *swboard)
{
	NateonSession *session;
	NateonMessage *msg;
	GList *l;

//#ifdef NATEON_DEBUG_SB
	purple_debug_info("nateon", "switchboard_destroy: swboard(%p)\n", swboard);
//#endif

	g_return_if_fail(swboard != NULL);

	if (swboard->destroying)
		return;

	swboard->destroying = TRUE;

//	/* If it linked us is because its looking for trouble */
//	if (swboard->slplink != NULL)
//		nateon_slplink_destroy(swboard->slplink);

	/* Destroy the message queue */
	while ((msg = g_queue_pop_head(swboard->msg_queue)) != NULL)
	{
//		if (swboard->error != NATEON_SB_ERROR_NONE)
//		{
//			/* The messages could not be sent due to a switchboard error */
//			msg_error_helper(swboard->cmdproc, msg,
//							 NATEON_MSG_ERROR_SB);
//		}
//		nateon_message_unref(msg);
	}

	g_queue_free(swboard->msg_queue);

//	/* msg_error_helper will both remove the msg from ack_list and
//	   unref it, so we don't need to do either here */
//	while ((l = swboard->ack_list) != NULL)
//		msg_error_helper(swboard->cmdproc, l->data, NATEON_MSG_ERROR_SB);

	if (swboard->im_user != NULL)
		g_free(swboard->im_user);

	if (swboard->auth_key != NULL)
		g_free(swboard->auth_key);

//	if (swboard->session_id != NULL)
//		g_free(swboard->session_id);

	for (l = swboard->users; l != NULL; l = l->next)
		g_free(l->data);

	session = swboard->session;
	session->switches = g_list_remove(session->switches, swboard);

//#if 0
//	/* This should never happen or we are in trouble. */
//	if (swboard->servconn != NULL)
//		nateon_servconn_destroy(swboard->servconn);
//#endif

	swboard->cmdproc->data = NULL;

	nateon_servconn_set_disconnect_cb(swboard->servconn, NULL);

	nateon_servconn_destroy(swboard->servconn);

	g_free(swboard);
}

void
nateon_switchboard_set_auth_key(NateonSwitchBoard *swboard, const char *key)
{
	g_return_if_fail(swboard != NULL);
	g_return_if_fail(key != NULL);

	swboard->auth_key = g_strdup(key);
}

//const char *
//nateon_switchboard_get_auth_key(NateonSwitchBoard *swboard)
//{
//	g_return_val_if_fail(swboard != NULL, NULL);
//
//	return swboard->auth_key;
//}
//
//void
//nateon_switchboard_set_session_id(NateonSwitchBoard *swboard, const char *id)
//{
//	g_return_if_fail(swboard != NULL);
//	g_return_if_fail(id != NULL);
//
//	if (swboard->session_id != NULL)
//		g_free(swboard->session_id);
//
//	swboard->session_id = g_strdup(id);
//}
//
//const char *
//nateon_switchboard_get_session_id(NateonSwitchBoard *swboard)
//{
//	g_return_val_if_fail(swboard != NULL, NULL);
//
//	return swboard->session_id;
//}

void
nateon_switchboard_set_invited(NateonSwitchBoard *swboard, gboolean invited)
{
	g_return_if_fail(swboard != NULL);

	swboard->invited = invited;
}

gboolean
nateon_switchboard_is_invited(NateonSwitchBoard *swboard)
{
	g_return_val_if_fail(swboard != NULL, FALSE);

	return swboard->invited;
}

/**************************************************************************
 * Utility
 **************************************************************************/

//static void
//send_clientcaps(NateonSwitchBoard *swboard)
//{
//	NateonMessage *msg;
//
//	msg = nateon_message_new(NATEON_MSG_CAPS);
//	nateon_message_set_content_type(msg, "text/x-clientcaps");
//	nateon_message_set_flag(msg, 'U');
//	nateon_message_set_bin_data(msg, NATEON_CLIENTINFO, strlen(NATEON_CLIENTINFO));
//
//	nateon_switchboard_send_msg(swboard, msg, TRUE);
//
//	nateon_message_destroy(msg);
//}

static void
nateon_switchboard_add_user(NateonSwitchBoard *swboard, const char *user)
{
	NateonCmdProc *cmdproc;
	PurpleAccount *account;

	g_return_if_fail(swboard != NULL);

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	cmdproc = swboard->cmdproc;
	account = cmdproc->session->account;

	swboard->users = g_list_prepend(swboard->users, g_strdup(user));
	swboard->current_users++;
	swboard->empty = FALSE;

//#ifdef NATEON_DEBUG_CHAT
//	purple_debug_info("nateon", "user=[%s], total=%d\n", user,
//					swboard->current_users);
//#endif

//	if (!(swboard->flag & NATEON_SB_FLAG_IM) && (swboard->conv != NULL))
//	{
//		/* This is a helper switchboard. */
//		purple_debug_error("nateon", "switchboard_add_user: conv != NULL\n");
//		return;
//	}

	if ((swboard->conv != NULL) &&
		(purple_conversation_get_type(swboard->conv) == PURPLE_CONV_TYPE_CHAT))
	{
		purple_conv_chat_add_user(PURPLE_CONV_CHAT(swboard->conv), user, NULL,
								PURPLE_CBFLAGS_NONE, TRUE);
	}
	else if (swboard->current_users > 1 || swboard->total_users > 1)
	{
		if (swboard->conv == NULL ||
			purple_conversation_get_type(swboard->conv) != PURPLE_CONV_TYPE_CHAT)
		{
			GList *l;

//#ifdef NATEON_DEBUG_CHAT
//			purple_debug_info("nateon", "[chat] Switching to chat.\n");
//#endif

//#if 0
//			/* this is bad - it causes nateon_switchboard_close to be called on the
//			 * switchboard we're in the middle of using :( */
//			if (swboard->conv != NULL)
//				purple_conversation_destroy(swboard->conv);
//#endif

			cmdproc->session->conv_seq++;
			swboard->chat_id = cmdproc->session->conv_seq;
//			swboard->flag |= NATEON_SB_FLAG_IM;
			swboard->conv = serv_got_joined_chat(account->gc, swboard->chat_id, "NATEON Chat");

			for (l = swboard->users; l != NULL; l = l->next)
			{
				const char *tmp_user;

				tmp_user = l->data;

//#ifdef NATEON_DEBUG_CHAT
//				purple_debug_info("nateon", "[chat] Adding [%s].\n", tmp_user);
//#endif

				purple_conv_chat_add_user(PURPLE_CONV_CHAT(swboard->conv), tmp_user, NULL, PURPLE_CBFLAGS_NONE, TRUE);
			}

//#ifdef NATEON_DEBUG_CHAT
//			purple_debug_info("nateon", "[chat] We add ourselves.\n");
//#endif

			purple_conv_chat_add_user(PURPLE_CONV_CHAT(swboard->conv),
									purple_account_get_username(account),
									NULL, PURPLE_CBFLAGS_NONE, TRUE);

			g_free(swboard->im_user);
			swboard->im_user = NULL;
		}
	}
	else if (swboard->conv == NULL)
	{
		swboard->conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
															user, account);
	}
	else
	{
		purple_debug_warning("nateon", "switchboard_add_user: This should not happen!\n");
	}
}

//static PurpleConversation *
//nateon_switchboard_get_conv(NateonSwitchBoard *swboard)
//{
//	PurpleAccount *account;
//
//	g_return_val_if_fail(swboard != NULL, NULL);
//
//	if (swboard->conv != NULL)
//		return swboard->conv;
//
//	purple_debug_error("nateon", "Switchboard with unassigned conversation\n");
//
//	account = swboard->session->account;
//
//	return (swboard->conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,
//												  account, swboard->im_user));
//}
//
//static void
//nateon_switchboard_report_user(NateonSwitchBoard *swboard, PurpleMessageFlags flags, const char *msg)
//{
//	PurpleConversation *conv;
//
//	g_return_if_fail(swboard != NULL);
//	g_return_if_fail(msg != NULL);
//
//	if ((conv = nateon_switchboard_get_conv(swboard)) != NULL)
//	{
//		purple_conversation_write(conv, NULL, msg, flags, time(NULL));
//	}
//}
//
//static void
//swboard_error_helper(NateonSwitchBoard *swboard, int reason, const char *account_name)
//{
//	g_return_if_fail(swboard != NULL);
//
//	purple_debug_warning("msg", "Error: Unable to call the user %s for reason %i\n",
//					   account_name ? account_name : "(null)", reason);
//
//	/* TODO: if current_users > 0, this is probably a chat and an invite failed,
//	 * we should report that in the chat or something */
//	if (swboard->current_users == 0)
//	{
//		swboard->error = reason;
//		nateon_switchboard_close(swboard);
//	}
//}
//
//static void
//cal_error_helper(NateonTransaction *trans, int reason)
//{
//	NateonSwitchBoard *swboard;
//	const char *account_name;
//	char **params;
//
//	params = g_strsplit(trans->params, " ", 0);
//
//	account_name = params[0];
//
//	swboard = trans->data;
//
//	purple_debug_warning("nateon", "cal_error_helper: command %s failed for reason %i\n",trans->command,reason);
//
//	swboard_error_helper(swboard, reason, account_name);
//
//	g_strfreev(params);
//}
//
//static void
//msg_error_helper(NateonCmdProc *cmdproc, NateonMessage *msg, NateonMsgErrorType error)
//{
//	NateonSwitchBoard *swboard;
//
//	g_return_if_fail(cmdproc != NULL);
//	g_return_if_fail(msg     != NULL);
//
//	if ((error != NATEON_MSG_ERROR_SB) && (msg->nak_cb != NULL))
//		msg->nak_cb(msg, msg->ack_data);
//
//	swboard = cmdproc->data;
//
//	/* This is not good, and should be fixed somewhere else. */
//	g_return_if_fail(swboard != NULL);
//
//	if (msg->type == NATEON_MSG_TEXT)
//	{
//		const char *format, *str_reason;
//		char *body_str, *body_enc, *pre, *post;
//
//#if 0
//		if (swboard->conv == NULL)
//		{
//			if (msg->ack_ref)
//				nateon_message_unref(msg);
//
//			return;
//		}
//#endif
//
//		if (error == NATEON_MSG_ERROR_TIMEOUT)
//		{
//			str_reason = _("Message may have not been sent "
//						   "because a timeout occurred:");
//		}
//		else if (error == NATEON_MSG_ERROR_SB)
//		{
//			switch (swboard->error)
//			{
//				case NATEON_SB_ERROR_OFFLINE:
//					str_reason = _("Message could not be sent, "
//								   "not allowed while invisible:");
//					break;
//				case NATEON_SB_ERROR_USER_OFFLINE:
//					str_reason = _("Message could not be sent "
//								   "because the user is offline:");
//					break;
//				case NATEON_SB_ERROR_CONNECTION:
//					str_reason = _("Message could not be sent "
//								   "because a connection error occurred:");
//					break;
//				case NATEON_SB_ERROR_TOO_FAST:
//					str_reason = _("Message could not be sent "
//								   "because we are sending too quickly:");
//					break;					
//				default:
//					str_reason = _("Message could not be sent "
//								   "because an error with "
//								   "the switchboard occurred:");
//					break;
//			}
//		}
//		else
//		{
//			str_reason = _("Message may have not been sent "
//						   "because an unknown error occurred:");
//		}
//
//		body_str = nateon_message_to_string(msg);
//		body_enc = g_markup_escape_text(body_str, -1);
//		g_free(body_str);
//
//		format = nateon_message_get_attr(msg, "X-MMS-IM-Format");
//		nateon_parse_format(format, &pre, &post);
//		body_str = g_strdup_printf("%s%s%s", pre ? pre : "",
//								   body_enc ? body_enc : "", post ? post : "");
//		g_free(body_enc);
//		g_free(pre);
//		g_free(post);
//
//		nateon_switchboard_report_user(swboard, PURPLE_MESSAGE_ERROR,
//									str_reason);
//		nateon_switchboard_report_user(swboard, PURPLE_MESSAGE_RAW,
//									body_str);
//
//		g_free(body_str);
//	}
//
//	/* If a timeout occures we will want the msg around just in case we
//	 * receive the ACK after the timeout. */
//	if (msg->ack_ref && error != NATEON_MSG_ERROR_TIMEOUT)
//	{
//		swboard->ack_list = g_list_remove(swboard->ack_list, msg);
//		nateon_message_unref(msg);
//	}
//}
//
///**************************************************************************
// * Message Stuff
// **************************************************************************/
//
///** Called when a message times out. */
//static void
//msg_timeout(NateonCmdProc *cmdproc, NateonTransaction *trans)
//{
//	NateonMessage *msg;
//
//	msg = trans->data;
//
//	msg_error_helper(cmdproc, msg, NATEON_MSG_ERROR_TIMEOUT);
//}
//
///** Called when we receive an error of a message. */
//static void
//msg_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
//{
//	msg_error_helper(cmdproc, trans->data, NATEON_MSG_ERROR_UNKNOWN);
//}
//
//#if 0
///** Called when we receive an ack of a special message. */
//static void
//msg_ack(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonMessage *msg;
//
//	msg = cmd->trans->data;
//
//	if (msg->ack_cb != NULL)
//		msg->ack_cb(msg->ack_data);
//
//	nateon_message_unref(msg);
//}
//
///** Called when we receive a nak of a special message. */
//static void
//msg_nak(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonMessage *msg;
//
//	msg = cmd->trans->data;
//
//	nateon_message_unref(msg);
//}
//#endif

static void
release_msg(NateonSwitchBoard *swboard, NateonMessage *msg)
{
	NateonCmdProc *cmdproc;
	NateonTransaction *trans;
	//char *payload;
	//gsize payload_len;

	g_return_if_fail(swboard != NULL);
	g_return_if_fail(msg     != NULL);

	purple_debug_info("nateon", "%s\n", msg);

	cmdproc = swboard->cmdproc;

//	payload = nateon_message_gen_payload(msg, &payload_len);

//#ifdef NATEON_DEBUG_SB
//	nateon_message_show_readable(msg, "SB SEND", FALSE);
//#endif

	trans = nateon_transaction_new(cmdproc, "MESG", "%s", msg);

	/* Data for callbacks */
	nateon_transaction_set_data(trans, msg);

//	if (msg->type == NATEON_MSG_TEXT)
//	{
//		msg->ack_ref = TRUE;
//		nateon_message_ref(msg);
//		swboard->ack_list = g_list_append(swboard->ack_list, msg);
//		nateon_transaction_set_timeout_cb(trans, msg_timeout);
//	}
//	else if (msg->type == NATEON_MSG_SLP)
//	{
//		msg->ack_ref = TRUE;
//		nateon_message_ref(msg);
//		swboard->ack_list = g_list_append(swboard->ack_list, msg);
//		nateon_transaction_set_timeout_cb(trans, msg_timeout);
//#if 0
//		if (msg->ack_cb != NULL)
//		{
//			nateon_transaction_add_cb(trans, "ACK", msg_ack);
//			nateon_transaction_add_cb(trans, "NAK", msg_nak);
//		}
//#endif
//	}
//
//	trans->payload = payload;
//	trans->payload_len = payload_len;

//	msg->trans = trans;

	nateon_cmdproc_send_trans(cmdproc, trans);
}

static void
queue_msg(NateonSwitchBoard *swboard, NateonMessage *msg)
{
	g_return_if_fail(swboard != NULL);
	g_return_if_fail(msg     != NULL);

	purple_debug_info("nateon", "Appending message to queue.\n");

	g_queue_push_tail(swboard->msg_queue, msg);

	//nateon_message_ref(msg);
}

static void
process_queue(NateonSwitchBoard *swboard)
{
	NateonMessage *msg;

	g_return_if_fail(swboard != NULL);

	purple_debug_info("nateon", "Processing queue\n");

	while ((msg = g_queue_pop_head(swboard->msg_queue)) != NULL)
	{
		purple_debug_info("nateon", "Sending message\n");
		release_msg(swboard, msg);
		//nateon_message_unref(msg);
	}
}

gboolean
nateon_switchboard_can_send(NateonSwitchBoard *swboard)
{
	g_return_val_if_fail(swboard != NULL, FALSE);

	if (swboard->empty || !g_queue_is_empty(swboard->msg_queue))
		return FALSE;

	return TRUE;
}

void
nateon_switchboard_send_msg(NateonSwitchBoard *swboard, NateonMessage *msg, gboolean queue)
{
	g_return_if_fail(swboard != NULL);
	g_return_if_fail(msg     != NULL);

	purple_debug_info("nateon", "%s - %s\n", __FUNCTION__, msg);
	if (nateon_switchboard_can_send(swboard))
	{
		purple_debug_info("nateon", "release_msg()\n");
		release_msg(swboard, msg);
	}
	else if (queue)
	{
		purple_debug_info("nateon", "queue_msg()\n");
		queue_msg(swboard, msg);
	}
}

///**************************************************************************
// * Switchboard Commands
// **************************************************************************/

//static void
//ans_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSwitchBoard *swboard;
//
//	swboard = cmdproc->data;
//	swboard->ready = TRUE;
//}

static void
quit_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
//	NateonSwitchBoard *swboard;
//	const char *user;
//
//	swboard = cmdproc->data;
//	user = cmd->params[1];
//
////	if (!(swboard->flag & NATEON_SB_FLAG_IM) && (swboard->conv != NULL))
////		purple_debug_error("nateon_switchboard", "bye_cmd: helper bug\n");
//
//	if (swboard->conv == NULL)
//	{
//		/* This is a helper switchboard */
//		nateon_switchboard_destroy(swboard);
//	}
//	else if ((swboard->current_users > 1) ||
//			 (purple_conversation_get_type(swboard->conv) == PURPLE_CONV_TYPE_CHAT))
//	{
//		/* This is a switchboard used for a chat */
//		purple_conv_chat_remove_user(PURPLE_CONV_CHAT(swboard->conv), user, NULL);
//		swboard->current_users--;
//		if (swboard->current_users == 0)
//			nateon_switchboard_destroy(swboard);
//	}
//	else
//	{
//		/* This is a switchboard used for a im session */
//		nateon_switchboard_destroy(swboard);
//	}
}

//static void
//iro_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	PurpleAccount *account;
//	PurpleConnection *gc;
//	NateonSwitchBoard *swboard;
//
//	account = cmdproc->session->account;
//	gc = account->gc;
//	swboard = cmdproc->data;
//
//	swboard->total_users = atoi(cmd->params[2]);
//
//	nateon_switchboard_add_user(swboard, cmd->params[3]);
//}
//
//static void
//joi_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session;
//	PurpleAccount *account;
//	PurpleConnection *gc;
//	NateonSwitchBoard *swboard;
//	const char *account_name;
//
//	account_name = cmd->params[0];
//
//	session = cmdproc->session;
//	account = session->account;
//	gc = account->gc;
//	swboard = cmdproc->data;
//
//	nateon_switchboard_add_user(swboard, account_name);
//
//	process_queue(swboard);
//
//	if (!session->http_method)
//		send_clientcaps(swboard);
//
//	if (swboard->closed)
//		nateon_switchboard_close(swboard);
//}

static void
join_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	PurpleAccount *account;
	PurpleConnection *gc;
	NateonSwitchBoard *swboard;
	const char *account_name;

	account_name = cmd->params[0];

	session = cmdproc->session;
	account = session->account;
	gc = account->gc;
	swboard = cmdproc->data;

	nateon_switchboard_add_user(swboard, account_name);

	process_queue(swboard);

//	if (!session->http_method)
//		send_clientcaps(swboard);

	if (swboard->closed)
		nateon_switchboard_close(swboard);
}

//static void
//msg_cmd_post(NateonCmdProc *cmdproc, NateonCommand *cmd, char *payload, size_t len)
//{
//	NateonMessage *msg;
//
//	msg = nateon_message_new_from_cmd(cmdproc->session, cmd);
//
//	nateon_message_parse_payload(msg, payload, len);
//#ifdef NATEON_DEBUG_SB
//	nateon_message_show_readable(msg, "SB RECV", FALSE);
//#endif
//
//	if (msg->remote_user != NULL)
//		g_free (msg->remote_user);
//
//	msg->remote_user = g_strdup(cmd->params[0]);
//	nateon_cmdproc_process_msg(cmdproc, msg);
//
//	nateon_message_destroy(msg);
//}
//
//static void
//msg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	cmdproc->servconn->payload_len = atoi(cmd->params[2]);
//	cmdproc->last_cmd->payload_cb = msg_cmd_post;
//}

static void
//msg_cmd(NateonCmdProc *cmdproc, NateonMessage *msg)
msg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	PurpleConnection *gc;
	NateonSwitchBoard *swboard;
//	const char *body;
	char *body_str;
//	char *body_enc;
	char *body_final;
//	size_t body_len;
	const char *account_name;
//	const char *value;

	gc = cmdproc->session->account->gc;
	swboard = cmdproc->data;

//	body = nateon_message_get_bin_data(msg, &body_len);
//	body_str = g_strndup(body, body_len);
//	body_enc = g_markup_escape_text(body_str, -1);
//	g_free(body_str);

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	account_name = cmd->params[1];
	body_str = cmd->params[3];

	{
		char **params;

		params = g_strsplit(body_str, "%09", 0);

		body_final = g_strdup(purple_url_decode(params[3]));
	}

	purple_debug_info("nateon", "%s - %s %s\n", __FUNCTION__, account_name, body_final);

//	account_name = msg->remote_user;

//	if (!strcmp(account_name, "messenger@microsoft.com") &&
//		strstr(body, "immediate security update"))
//	{
//		return;
//	}

#if 0
	if ((value = nateon_message_get_attr(msg, "User-Agent")) != NULL)
	{
		purple_debug_misc("nateon", "User-Agent = '%s'\n", value);
	}
#endif

//	if ((value = nateon_message_get_attr(msg, "X-MMS-IM-Format")) != NULL)
//	{
//		char *pre, *post;
//
//		nateon_parse_format(value, &pre, &post);
//
//		body_final = g_strdup_printf("%s%s%s", pre ? pre : "",
//									 body_enc ? body_enc : "", post ? post : "");
//
//		g_free(pre);
//		g_free(post);
//		g_free(body_enc);
//	}
//	else
//	{
//		body_final = body_enc;
//	}
//
	swboard->flag |= NATEON_SB_FLAG_IM;

	if (swboard->current_users > 1 ||
		((swboard->conv != NULL) &&
		 purple_conversation_get_type(swboard->conv) == PURPLE_CONV_TYPE_CHAT))
	{
		/* If current_users is always ok as it should then there is no need to
		 * check if this is a chat. */
		if (swboard->current_users <= 1)
			purple_debug_misc("nateon", "plain_msg: current_users(%d)\n",
							swboard->current_users);

		serv_got_chat_in(gc, swboard->chat_id, account_name, 0, body_final,
						 time(NULL));
		if (swboard->conv == NULL)
		{
			swboard->conv = purple_find_chat(gc, swboard->chat_id);
			swboard->flag |= NATEON_SB_FLAG_IM;
		}
	}
	else
	{
		serv_got_im(gc, account_name, body_final, 0, time(NULL));
		if (swboard->conv == NULL)
		{
			swboard->conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
									account_name, purple_connection_get_account(gc));
			swboard->flag |= NATEON_SB_FLAG_IM;
		}
	}

	g_free(body_final);
}

static void
mesg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	PurpleConnection *gc;
	NateonSwitchBoard *swboard;
	const char *subcmd;

	//g_return_if_fail(cmd->param_count > 2);
	if (cmd->param_count < 3)
		return;

	subcmd = cmd->params[2];

	purple_debug_info("nateon", "%s - %s %s\n", __FUNCTION__, cmd->params[1], cmd->params[3]);

	if (!strcmp(subcmd, "MSG"))
	{
		msg_cmd(cmdproc, cmd);
	}
	else if (!strcmp(subcmd, "TYPING"))
	{
		gc = cmdproc->session->account->gc;
		swboard = cmdproc->data;
		serv_got_typing(gc, swboard->im_user, NATEON_TYPING_RECV_TIMEOUT, PURPLE_TYPING);

	}
}

//static void
//nak_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonMessage *msg;
//
//	msg = cmd->trans->data;
//	g_return_if_fail(msg != NULL);
//
//	msg_error_helper(cmdproc, msg, NATEON_MSG_ERROR_NAK);
//}
//
//static void
//ack_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSwitchBoard *swboard;
//	NateonMessage *msg;
//
//	msg = cmd->trans->data;
//
//	if (msg->ack_cb != NULL)
//		msg->ack_cb(msg, msg->ack_data);
//
//	swboard = cmdproc->data;
//	swboard->ack_list = g_list_remove(swboard->ack_list, msg);
//	nateon_message_unref(msg);
//}
//
//static void
//out_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	PurpleConnection *gc;
//	NateonSwitchBoard *swboard;
//
//	gc = cmdproc->session->account->gc;
//	swboard = cmdproc->data;
//
//	if (swboard->current_users > 1)
//		serv_got_chat_left(gc, swboard->chat_id);
//
//	nateon_switchboard_disconnect(swboard);
//}

//static void
//quit_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	PurpleConnection *gc;
//	NateonSwitchBoard *swboard;
//
//	gc = cmdproc->session->account->gc;
//	swboard = cmdproc->data;
//
//	if (swboard->current_users > 1)
//		serv_got_chat_left(gc, swboard->chat_id);
//
//	nateon_switchboard_disconnect(swboard);
//}

static void
user_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSwitchBoard *swboard;

	swboard = cmdproc->data;

#if 0
	GList *l;

	for (l = swboard->users; l != NULL; l = l->next)
	{
		const char *user;
		user = l->data;

		nateon_cmdproc_send(cmdproc, "CAL", "%s", user);
	}
#endif

	swboard->ready = TRUE;
	nateon_cmdproc_process_queue(cmdproc);
}

///**************************************************************************
// * Message Handlers
// **************************************************************************/
//static void
//plain_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	PurpleConnection *gc;
//	NateonSwitchBoard *swboard;
//	const char *body;
//	char *body_str;
//	char *body_enc;
//	char *body_final;
//	size_t body_len;
//	const char *account_name;
//	const char *value;
//
//	gc = cmdproc->session->account->gc;
//	swboard = cmdproc->data;
//
//	body = nateon_message_get_bin_data(msg, &body_len);
//	body_str = g_strndup(body, body_len);
//	body_enc = g_markup_escape_text(body_str, -1);
//	g_free(body_str);
//
//	account_name = msg->remote_user;
//
//	if (!strcmp(account_name, "messenger@microsoft.com") &&
//		strstr(body, "immediate security update"))
//	{
//		return;
//	}
//
//#if 0
//	if ((value = nateon_message_get_attr(msg, "User-Agent")) != NULL)
//	{
//		purple_debug_misc("nateon", "User-Agent = '%s'\n", value);
//	}
//#endif
//
//	if ((value = nateon_message_get_attr(msg, "X-MMS-IM-Format")) != NULL)
//	{
//		char *pre, *post;
//
//		nateon_parse_format(value, &pre, &post);
//
//		body_final = g_strdup_printf("%s%s%s", pre ? pre : "",
//									 body_enc ? body_enc : "", post ? post : "");
//
//		g_free(pre);
//		g_free(post);
//		g_free(body_enc);
//	}
//	else
//	{
//		body_final = body_enc;
//	}
//
//	swboard->flag |= NATEON_SB_FLAG_IM;
//
//	if (swboard->current_users > 1 ||
//		((swboard->conv != NULL) &&
//		 purple_conversation_get_type(swboard->conv) == PURPLE_CONV_TYPE_CHAT))
//	{
//		/* If current_users is always ok as it should then there is no need to
//		 * check if this is a chat. */
//		if (swboard->current_users <= 1)
//			purple_debug_misc("nateon", "plain_msg: current_users(%d)\n",
//							swboard->current_users);
//
//		serv_got_chat_in(gc, swboard->chat_id, account_name, 0, body_final,
//						 time(NULL));
//		if (swboard->conv == NULL)
//		{
//			swboard->conv = purple_find_chat(gc, swboard->chat_id);
//			swboard->flag |= NATEON_SB_FLAG_IM;
//		}
//	}
//	else
//	{
//		serv_got_im(gc, account_name, body_final, 0, time(NULL));
//		if (swboard->conv == NULL)
//		{
//			swboard->conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
//									account_name, purple_connection_get_account(gc));
//			swboard->flag |= NATEON_SB_FLAG_IM;
//		}
//	}
//
//	g_free(body_final);
//}
//
//static void
//control_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	PurpleConnection *gc;
//	NateonSwitchBoard *swboard;
//	const char *value;
//	char *account_name;
//
//	gc = cmdproc->session->account->gc;
//	swboard = cmdproc->data;
//	account_name = msg->remote_user;
//
//	if (swboard->current_users == 1 &&
//		(value = nateon_message_get_attr(msg, "TypingUser")) != NULL)
//	{
//		serv_got_typing(gc, account_name, NATEON_TYPING_RECV_TIMEOUT,
//						PURPLE_TYPING);
//	}
//}
//
//static void
//clientcaps_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//#if 0
//	NateonSession *session;
//	NateonSwitchBoard *swboard;
//	NateonUser *user;
//	GHashTable *clientcaps;
//	const char *value;
//
//	char *account_name = msg->sender;
//
//	session = cmdproc->session;
//	swboard = cmdproc->servconn->swboard;
//
//	clientcaps = nateon_message_get_hashtable_from_body(msg);
//#endif
//}
//
//static void
//nudge_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	NateonSwitchBoard *swboard;
//	char *username, *str;
//	PurpleAccount *account;
//	PurpleBuddy *buddy;
//	const char *user;
//
//	swboard = cmdproc->data;
//	account = cmdproc->session->account;
//	user = msg->remote_user;
//
//	if ((buddy = purple_find_buddy(account, user)) != NULL)
//		username = g_markup_escape_text(purple_buddy_get_alias(buddy), -1);
//	else
//		username = g_markup_escape_text(user, -1);
//
//	str = g_strdup_printf(_("%s just sent you a Nudge!"), username);
//	g_free(username);
//	nateon_switchboard_report_user(swboard, PURPLE_MESSAGE_SYSTEM, str);
//	g_free(str);
//}

/**************************************************************************
 * Connect stuff
 **************************************************************************/
static void
connect_cb(NateonServConn *servconn)
{
	NateonSwitchBoard *swboard;
	NateonCmdProc *cmdproc;
	PurpleAccount *account;

	purple_debug_info("nateon", "switchboard - %s\n", __FUNCTION__);

	cmdproc = servconn->cmdproc;
	g_return_if_fail(cmdproc != NULL);

	account = cmdproc->session->account;
	swboard = cmdproc->data;
	g_return_if_fail(swboard != NULL);

	if (nateon_switchboard_is_invited(swboard))
	{
		NateonUser *user = cmdproc->session->user;

		purple_debug_info("nateon", "== invited ==\n");
		swboard->empty = FALSE;

		nateon_cmdproc_send(cmdproc, "ENTR", "%s %s %s %s UTF8 P",
						 purple_account_get_username(account),
						 user->store_name,
						 user->friendly_name,
						 swboard->auth_key);
	}
	else
	{
		NateonUser *user = cmdproc->session->user;

		nateon_cmdproc_send(cmdproc, "ENTR", "%s %s %s %s UTF8 P",
						 purple_account_get_username(account),
						 user->store_name,
						 user->friendly_name,
						 swboard->auth_key);
	}
}

static void
disconnect_cb(NateonServConn *servconn)
{
	NateonSwitchBoard *swboard;

	swboard = servconn->cmdproc->data;
	g_return_if_fail(swboard != NULL);

	nateon_servconn_set_disconnect_cb(swboard->servconn, NULL);

	nateon_switchboard_destroy(swboard);
}

gboolean nateon_switchboard_connect(NateonSwitchBoard *swboard, const char *host, int port)
{
	g_return_val_if_fail(swboard != NULL, FALSE);

	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	purple_debug_info("nateon", "host %s\n", host);
	purple_debug_info("nateon", "port %d\n", port);

	nateon_servconn_set_connect_cb(swboard->servconn, connect_cb);
	nateon_servconn_set_disconnect_cb(swboard->servconn, disconnect_cb);

	return nateon_servconn_connect(swboard->servconn, host, port);
}

void nateon_switchboard_disconnect(NateonSwitchBoard *swboard)
{
	g_return_if_fail(swboard != NULL);

	nateon_servconn_disconnect(swboard->servconn);
}

/**************************************************************************
 * Call stuff
 **************************************************************************/
//static void
//got_cal(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//#if 0
//	NateonSwitchBoard *swboard;
//	const char *user;
//
//	swboard = cmdproc->data;
//
//	user = cmd->params[0];
//
//	nateon_switchboard_add_user(swboard, user);
//#endif
//}
//
//static void
//cal_timeout(NateonCmdProc *cmdproc, NateonTransaction *trans)
//{
//	purple_debug_warning("nateon", "cal_timeout: command %s timed out\n", trans->command);
//
//	cal_error_helper(trans, NATEON_SB_ERROR_UNKNOWN);
//}
//
//static void
//cal_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
//{
//	int reason = NATEON_SB_ERROR_UNKNOWN;
//
//	if (error == 215)
//	{
//		purple_debug_info("nateon", "Invited user already in switchboard\n");
//		return;
//	}
//	else if (error == 217)
//	{
//		reason = NATEON_SB_ERROR_USER_OFFLINE;
//	}
//
//	purple_debug_warning("nateon", "cal_error: command %s gave error %i\n", trans->command, error);
//
//	cal_error_helper(trans, reason);
//}

void
nateon_switchboard_request_add_user(NateonSwitchBoard *swboard, const char *user)
{
	NateonTransaction *trans;
	NateonCmdProc *cmdproc;
	NateonSession *session;
	NateonServConn *servconn;
	PurpleAccount *account;
	const char *username;

	g_return_if_fail(swboard != NULL);

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	cmdproc = swboard->cmdproc;
	session = cmdproc->session;
	servconn = swboard->servconn;
	account = session->account;

	username = purple_account_get_username(account);

	trans = nateon_transaction_new(cmdproc, "ENTR", "%s %s %s %s UTF8 P", username, "1" "1", swboard->auth_key);
	/* this doesn't do anything, but users seem to think that
	 * 'Unhandled command' is some kind of error, so we don't report it */
//	nateon_transaction_add_cb(trans, "PACK", got_cal);

	nateon_transaction_set_data(trans, swboard);
//	nateon_transaction_set_timeout_cb(trans, cal_timeout);

//	if (swboard->ready)
		nateon_cmdproc_send_trans(cmdproc, trans);
//	else
//		nateon_cmdproc_queue_trans(cmdproc, trans);
}

/**************************************************************************
 * Create & Transfer stuff
 **************************************************************************/

static void
got_swboard(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSwitchBoard *swboard;
	char *host;
	int port;
	swboard = cmd->trans->data;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	if (g_list_find(cmdproc->session->switches, swboard) == NULL)
		/* The conversation window was closed. */
		return;

	nateon_switchboard_set_auth_key(swboard, cmd->params[3]);

	host = g_strdup(cmd->params[1]);
	port = 5004; // cmd->params[2]
//	nateon_parse_socket(cmd->params[2], &host, &port);

	if (!nateon_switchboard_connect(swboard, host, port))
		nateon_switchboard_destroy(swboard);

	{
		NateonCmdProc *cmdproc;
		NateonSession *session = swboard->session;
		NateonServConn *servconn;
		PurpleAccount *account = session->account;
		const char *username;
		NateonUser *user;

		int len;

		cmdproc = swboard->session->notification->cmdproc;
		servconn = cmdproc->servconn;
		
		user = nateon_userlist_find_user_with_name(session->userlist, swboard->im_user);
		username = purple_account_get_username(account);

		len = 8 + strlen(username) + strlen(host) + strlen(cmd->params[2]) + strlen(swboard->auth_key);

		nateon_cmdproc_send(cmdproc, "CTOC", "%s %s %d\r\nINVT %s %s %d %s", swboard->im_user, user->status, len, username, host, port, swboard->auth_key);

	}

	g_free(host);
}

//static void
//xfr_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
//{
//	NateonSwitchBoard *swboard;
//	int reason = NATEON_SB_ERROR_UNKNOWN;
//
//	if (error == 913)
//		reason = NATEON_SB_ERROR_OFFLINE;
//	else if (error == 800)
//		reason = NATEON_SB_ERROR_TOO_FAST;
//
//	swboard = trans->data;
//
//	purple_debug_info("nateon", "xfr_error %i for %s: trans %x, command %s, reason %i\n",
//					error, (swboard->im_user ? swboard->im_user : "(null)"), trans,
//					(trans->command ? trans->command : "(null)"), reason);
//
//	swboard_error_helper(swboard, reason, swboard->im_user);
//}

void
nateon_switchboard_request(NateonSwitchBoard *swboard)
{
	NateonCmdProc *cmdproc;
	NateonTransaction *trans;

	g_return_if_fail(swboard != NULL);

	cmdproc = swboard->session->notification->cmdproc;

	trans = nateon_transaction_new(cmdproc, "RESS", NULL);
	nateon_transaction_add_cb(trans, "RESS", got_swboard);

	nateon_transaction_set_data(trans, swboard);
//	nateon_transaction_set_error_cb(trans, xfr_error);

	nateon_cmdproc_send_trans(cmdproc, trans);
}

void
nateon_switchboard_close(NateonSwitchBoard *swboard)
{
	g_return_if_fail(swboard != NULL);

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

//	if (swboard->error != NATEON_SB_ERROR_NONE)
//	{
//		nateon_switchboard_destroy(swboard);
//	}
//	else if (g_queue_is_empty(swboard->msg_queue) ||
	if (g_queue_is_empty(swboard->msg_queue) ||
			 !swboard->session->connected)
	{
		NateonCmdProc *cmdproc;
		cmdproc = swboard->cmdproc;
		//nateon_cmdproc_send_quick(cmdproc, "QUIT", NULL, NULL);
		nateon_cmdproc_send(cmdproc, "QUIT", NULL, NULL);

		nateon_switchboard_destroy(swboard);
	}
	else
	{
		swboard->closed = TRUE;
	}
}

gboolean
nateon_switchboard_release(NateonSwitchBoard *swboard, NateonSBFlag flag)
{
	g_return_val_if_fail(swboard != NULL, FALSE);

	swboard->flag &= ~flag;

	if (flag == NATEON_SB_FLAG_IM)
		/* Forget any conversation that used to be associated with this
		 * swboard. */
		swboard->conv = NULL;

	if (swboard->flag == 0)
	{
		nateon_switchboard_close(swboard);
		return TRUE;
	}

	return FALSE;
}

/**************************************************************************
 * Init stuff
 **************************************************************************/

void
nateon_switchboard_init(void)
{
	cbs_table = nateon_table_new();

	nateon_table_add_cmd(cbs_table, "ENTR", "ENTR", NULL);
//	nateon_table_add_cmd(cbs_table, "ANS", "ANS", ans_cmd);
//	nateon_table_add_cmd(cbs_table, "ANS", "IRO", iro_cmd);
//
//	nateon_table_add_cmd(cbs_table, "MSG", "ACK", ack_cmd);
//	nateon_table_add_cmd(cbs_table, "MSG", "NAK", nak_cmd);
//
//	nateon_table_add_cmd(cbs_table, "USR", "USR", usr_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "USER", user_cmd);
//
//	nateon_table_add_cmd(cbs_table, NULL, "MSG", msg_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "MESG", mesg_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "JOI", joi_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "JOIN", join_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "BYE", bye_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "OUT", out_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "QUIT", quit_cmd);
//
//#if 0
//	/* They might skip the history */
//	nateon_table_add_cmd(cbs_table, NULL, "ACK", NULL);
//#endif
//
//	nateon_table_add_error(cbs_table, "MSG", msg_error);
//	nateon_table_add_error(cbs_table, "CAL", cal_error);
//
//	/* Register the message type callbacks. */
//	nateon_table_add_msg_type(cbs_table, "text/plain",
//						   plain_msg);
//	nateon_table_add_msg_type(cbs_table, "text/x-msmsgscontrol",
//						   control_msg);
//	nateon_table_add_msg_type(cbs_table, "text/x-clientcaps",
//						   clientcaps_msg);
//	nateon_table_add_msg_type(cbs_table, "text/x-clientinfo",
//						   clientcaps_msg);
//	nateon_table_add_msg_type(cbs_table, "application/x-nateonmsgrp2p",
//						   nateon_p2p_msg);
//	nateon_table_add_msg_type(cbs_table, "text/x-mms-emoticon",
//						   nateon_emoticon_msg);
//	nateon_table_add_msg_type(cbs_table, "text/x-mms-animemoticon",
//	                                           nateon_emoticon_msg);
//	nateon_table_add_msg_type(cbs_table, "text/x-nateonmsgr-datacast",
//						   nudge_msg);
//#if 0
//	nateon_table_add_msg_type(cbs_table, "text/x-msmmsginvite",
//						   nateon_invite_msg);
//#endif
}

void
nateon_switchboard_end(void)
{
	nateon_table_destroy(cbs_table);
}
