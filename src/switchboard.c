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
#include "prefs.h"
#include "switchboard.h"
#include "notification.h"
#include "nateon-utils.h"
#include "smiley.h"
#include "xfer.h"

#include "error.h"

static NateonTable *cbs_table;

/**
 * Volatile emoticon, just for this switchboard, not saved to disk.
 */
typedef struct _SwboardEmoticon
{
	guchar *data;
	size_t size;
} SwboardEmoticon;

/**************************************************************************
 * Prototypes
 **************************************************************************/
static void msg_error_helper(NateonCmdProc *cmdproc, NateonMessage *msg,
							 NateonMsgErrorType error);

static void nateon_swboard_emoticon_destroy( gpointer ptr );

static void emoticon_done_download(NateonCmdProc *cmdproc);

gboolean
is_int( const char *str );

gchar *no_dp( const gchar *id );

SwboardEmoticon*
nateon_conv_custom_smiley_find( NateonSwitchBoard *swboard, const char *shortcut );

static void
nateon_conv_custom_smiley_draw( NateonCmdProc *cmdproc, const char *msg );

static void
nateon_conv_custom_smiley_add( NateonSwitchBoard *swboard,
	const char *shortcut, const guchar *img_data, size_t img_sz );

static void
emoticon_timeout(NateonCmdProc *cmdproc, NateonTransaction *trans);

/**************************************************************************
 * helper functions
 **************************************************************************/

static void
emoticon_done_download(NateonCmdProc *cmdproc)
{
	cmdproc->emoticon_download_cnt--; // cancel transfer and allow message to be run.
	#if 1
	purple_debug_info("nateon", "[%s] %d emoticon downloads in progress\n",
		__FUNCTION__, cmdproc->emoticon_download_cnt);
	if( cmdproc->emoticon_download_cnt == 0 )
	{
		// all downloads done. process rxqueue
		nateon_cmdproc_process_rxqueue(cmdproc);
	}
	#endif
}

/**
 * Destroy a SwboardEmoticon.
 */
static void nateon_swboard_emoticon_destroy( gpointer ptr )
{
	SwboardEmoticon *im = ptr;

	// free data
	if( im->data )
		g_free( im->data );
	
	// free itself
	g_free( ptr );
}

/**
 * Check if given str matches regexp -?[0-9]+
 * Hmm... I suspect that libpurple or glib has something like this,
 * but I can't find it.
 */
gboolean
is_int( const char *str )
{   
	long int len = strlen(str);
	char *end;
	strtol(str, &end, 10);
	return (end-str) == len;
}

/**
 * input: ssss@nate.com|dpc_03805:12876|X0HA
 * output: ssss@nate.com
 * The output must be g_free()'ed when no longer needed.
 */
gchar *no_dp( const gchar *id )
{
	gchar *ret = g_strdup(id);
	gchar *pos = g_strstr_len(ret, -1, "|");
	if( pos )
		*pos = '\0';
	return ret;
}

/**
 * Scan the hash table for list of shortcuts.
 * For each shortcut, look it up in the msg and if it should be
 * drawn to UI, draw it.
 */
static void
nateon_conv_custom_smiley_draw( NateonCmdProc *cmdproc, const char *msg )
{
	GHashTableIter iter;
	gpointer key, value;
	NateonSwitchBoard *swboard;
	PurpleConversation *ps;

	g_return_if_fail( cmdproc );

	swboard = cmdproc->data;
	g_return_if_fail( swboard );
	ps = swboard->conv;
	g_return_if_fail( swboard->emo_table );

	purple_debug_info("nateon", "[%s:%d]\n", __FUNCTION__, __LINE__);

	// copied from M$N messenger code:
	/* If the conversation doesn't exist then this is a custom smiley
	 * used in the first message in a conversation: we need to create
	 * the conversation now, otherwise the custom smiley won't be shown.
	 * This happens because every GtkIMHtml has its own smiley tree: if
	 * the conversation doesn't exist then we cannot associate the new
	 * smiley with its GtkIMHtml widget. */
	if (!ps) {
		ps = purple_conversation_new(PURPLE_CONV_TYPE_IM,
			cmdproc->session->account, swboard->im_user);
		// Hmm, it seems that ps is not getting freed, for M$N messenger.
		// I tried freeing it then the first MESSAGE (not just emoticon)
		// goes missing forever.
	}

	// iterate over all custom emoticon shortcuts
	g_hash_table_iter_init(&iter, swboard->emo_table);
	while (g_hash_table_iter_next (&iter, &key, &value)) 
	{
		const char *shortcut = key;
		SwboardEmoticon *se = value;

		// if emoticon should be drawn to GUI, draw it.
		if( purple_conv_custom_smiley_add(ps, shortcut, "sha1", "", TRUE ) )
		{
			purple_debug_info("nateon", "[%s:%d] drawing %s\n",
				__FUNCTION__, __LINE__, shortcut);
			purple_conv_custom_smiley_write( ps, shortcut, se->data, se->size );
			purple_conv_custom_smiley_close( ps, shortcut );
		}
	}
};

/**************************************************************************
 * Main
 **************************************************************************/

/**
 * Return true if emoticon with shortcut is downloaded, otherwise false.
 */
SwboardEmoticon*
nateon_conv_custom_smiley_find( NateonSwitchBoard *swboard, const char *shortcut )
{
	return g_hash_table_lookup( swboard->emo_table, shortcut );
}

/**
 * Make EMFL data for emoticon transfer.
 * returns:
 * EMFL filename filesz shortcut [binarydata].
 * @param [out] sz emfl_sz: sizeof(emfl)
 */
static char*
make_emfl( size_t *emfl_sz, const char *shortcut )
{
	char *emfl = NULL;
	char *header;
	size_t img_sz;
	size_t header_sz;

	PurpleSmiley *ps = purple_smileys_find_by_shortcut(shortcut);
	PurpleStoredImage* img = purple_smiley_get_stored_image(ps);
	img_sz = purple_imgstore_get_size(img);

	header =  g_strdup_printf("EMFL\t%s\t%ld\t%s\t",
		purple_imgstore_get_filename(img), img_sz, shortcut);
	header_sz = strlen(header);

	*emfl_sz = header_sz + img_sz;
	emfl = g_new( char, *emfl_sz );
	g_memmove( emfl, header, header_sz );
	g_memmove( &emfl[header_sz], purple_imgstore_get_data(img), img_sz );

	#if 0
	{
		int i;
		for( i = 0 ; i < *emfl_sz ; ++i )
		{
			printf( "%c", emfl[i] );
		}
		printf( "\n" );
	}
	#endif
	
	g_free(header);
	return emfl;
}

/**
 * Add downloaded image to our hash table.
 */
static void
nateon_conv_custom_smiley_add( NateonSwitchBoard *swboard,
	const char *shortcut, const guchar *img_data, size_t img_sz )
{
	SwboardEmoticon *se;
	g_return_if_fail( swboard );
	g_return_if_fail( swboard->emo_table );

	se = g_new0(SwboardEmoticon, 1);
	se->data = g_memdup(img_data, img_sz);
	se->size = img_sz;

	g_hash_table_insert( swboard->emo_table, g_strdup(shortcut), se );
}

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
	swboard->emo_table = g_hash_table_new_full(g_str_hash,
		g_str_equal, g_free, nateon_swboard_emoticon_destroy);

	session->switches = g_list_append(session->switches, swboard);

	return swboard;
}

void
nateon_switchboard_destroy(NateonSwitchBoard *swboard)
{
	NateonSession *session;
	NateonMessage *msg;
	GList *l;

#ifdef NATEON_DEBUG_SB
	purple_debug_info("nateon", "switchboard_destroy: swboard(%p)\n", swboard);
#endif

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
		if (swboard->error != NATEON_SB_ERROR_NONE)
		{
			/* The messages could not be sent due to a switchboard error */
			msg_error_helper(swboard->cmdproc, msg,
							 NATEON_MSG_ERROR_SB);
		}
		nateon_message_unref(msg);
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
	
	if( swboard->emo_table )
		g_hash_table_destroy(swboard->emo_table);

//	if (swboard->session_id != NULL)
//		g_free(swboard->session_id);

	for (l = swboard->users; l != NULL; l = l->next)
		g_free(l->data);

	session = swboard->session;
	session->switches = g_list_remove(session->switches, swboard);

#if 0
	/* This should never happen or we are in trouble. */
	if (swboard->servconn != NULL)
		nateon_servconn_destroy(swboard->servconn);
#endif

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

	purple_debug_info("nateon", "[%s:%d]\n", __FUNCTION__, __LINE__);

	g_return_if_fail(swboard != NULL);

	cmdproc = swboard->cmdproc;
	account = cmdproc->session->account;

	swboard->users = g_list_prepend(swboard->users, g_strdup(user));
	swboard->current_users++;
	swboard->empty = FALSE;

#ifdef NATEON_DEBUG_CHAT
	purple_debug_info("nateon", "user=[%s], total=%d\n", user,
					swboard->current_users);
#endif

	if (!(swboard->flag & NATEON_SB_FLAG_IM) && (swboard->conv != NULL))
	{
		/* This is a helper switchboard. */
		purple_debug_error("nateon", "switchboard_add_user: conv != NULL\n");
		return;
	}

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

#ifdef NATEON_DEBUG_CHAT
			purple_debug_info("nateon", "[chat] Switching to chat.\n");
#endif

#if 0
			/* this is bad - it causes nateon_switchboard_close to be called on the
			 * switchboard we're in the middle of using :( */
			if (swboard->conv != NULL)
				purple_conversation_destroy(swboard->conv);
#endif

			cmdproc->session->conv_seq++;
			swboard->chat_id = cmdproc->session->conv_seq;
			swboard->flag |= NATEON_SB_FLAG_IM;
			swboard->conv = serv_got_joined_chat(account->gc, swboard->chat_id, "NATEON Chat");

			for (l = swboard->users; l != NULL; l = l->next)
			{
				const char *tmp_user;

				tmp_user = l->data;

#ifdef NATEON_DEBUG_CHAT
				purple_debug_info("nateon", "[chat] Adding [%s].\n", tmp_user);
#endif

				purple_conv_chat_add_user(PURPLE_CONV_CHAT(swboard->conv), tmp_user, NULL, PURPLE_CBFLAGS_NONE, TRUE);
			}

#ifdef NATEON_DEBUG_CHAT
			purple_debug_info("nateon", "[chat] We add ourselves.\n");
#endif

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

static void
swboard_error_helper(NateonSwitchBoard *swboard, int reason, const char *account_name)
{
	g_return_if_fail(swboard != NULL);

	purple_debug_warning("msg", "Error: Unable to call the user %s for reason %i\n",
					   account_name ? account_name : "(null)", reason);

	/* TODO: if current_users > 0, this is probably a chat and an invite failed,
	 * we should report that in the chat or something */
	if (swboard->current_users == 0)
	{
		swboard->error = reason;
		nateon_switchboard_close(swboard);
	}
}

static void
invt_error_helper(NateonTransaction *trans, int reason)
{
	NateonSwitchBoard *swboard;
	const char *account_name;
	char **params;

	params = g_strsplit(trans->params, " ", 0);

	account_name = params[0];

	swboard = trans->data;

	purple_debug_warning("nateon", "invt_error_helper: command %s failed for reason %i\n",trans->command,reason);

	swboard_error_helper(swboard, reason, account_name);

	g_strfreev(params);
}

static void
msg_error_helper(NateonCmdProc *cmdproc, NateonMessage *msg, NateonMsgErrorType error)
{
	// NateonSwitchBoard *swboard;

	g_return_if_fail(cmdproc != NULL);
	g_return_if_fail(msg     != NULL);

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

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
}

/**************************************************************************
 * Message Stuff
 **************************************************************************/

/** Called when a message times out. */
static void
msg_timeout(NateonCmdProc *cmdproc, NateonTransaction *trans)
{
	NateonMessage *msg;

	msg = trans->data;

	msg_error_helper(cmdproc, msg, NATEON_MSG_ERROR_TIMEOUT);
}

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

	purple_debug_info("nateon", "%p\n", msg);

	g_return_if_fail(swboard != NULL);
	g_return_if_fail(msg     != NULL);

	cmdproc = swboard->cmdproc;

//	payload = nateon_message_gen_payload(msg, &payload_len);

//#ifdef NATEON_DEBUG_SB
//	nateon_message_show_readable(msg, "SB SEND", FALSE);
//#endif

	trans = nateon_transaction_new(cmdproc, "MESG", "%s", msg->body);

	/* Data for callbacks */
	nateon_transaction_set_data(trans, msg);

	if (msg->type == NATEON_MSG_TEXT)
	{
		msg->ack_ref = TRUE;
		nateon_message_ref(msg);
		swboard->ack_list = g_list_append(swboard->ack_list, msg);
		nateon_transaction_set_timeout_cb(trans, msg_timeout);
	}
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

//	trans->payload = payload;
//	trans->payload_len = payload_len;

	msg->trans = trans;

	nateon_cmdproc_send_trans(cmdproc, trans);
}

static void
queue_msg(NateonSwitchBoard *swboard, NateonMessage *msg)
{
	g_return_if_fail(swboard != NULL);
	g_return_if_fail(msg     != NULL);

	purple_debug_info("nateon", "Appending message to queue.\n");

	g_queue_push_tail(swboard->msg_queue, msg);

	nateon_message_ref(msg);
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
		nateon_message_unref(msg);
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

	if (nateon_switchboard_can_send(swboard))
		release_msg(swboard, msg);
	else if (queue)
		queue_msg(swboard, msg);
}

///**************************************************************************
// * Switchboard Commands
// **************************************************************************/

static void
entr_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSwitchBoard *swboard;
	NateonNotification *notification;

	swboard = cmdproc->data;
	swboard->ready = TRUE;
	notification = swboard->session->notification;

	nateon_cmdproc_process_txqueue(notification->cmdproc);
}

/**
 * Switchboard CTOC, different from session CTOC.
 */
static void
swb_ctoc_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	payload_cmd(cmdproc, cmd, 3);
}

/**
 * EMFL command
 * EMoticon FiLe command?
 */
static void
swb_emfl_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	// Expected cmd->bin_data format:
	//
	// EMFL\t (this command)
		// NCE20110221112917.bmp\t (filename)... not needed!
		// 1498\t (binary data size)
		// :_\t (shortcut)
		// [binary data]

	int i, hold;
	char *shortcut;

	size_t sz = cmd->bin_data_sz;
	char *data = cmd->bin_data;

	size_t pic_sz;
	const guchar *pic_data;

	NateonSwitchBoard *swboard = cmdproc->data;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	for( i = 0 ; i < sz && data[i] != '\t' ; ++i ); // find 1st tab. (EMFL)


	for( ++i ; i < sz && data[i] != '\t' ; ++i ); // find 2nd tab. (filename)

	++i;
	hold = i;
	for( ; i < sz && data[i] != '\t' ; ++i ); // find 3rd tab. (real data size)

	// extract size information
	data[i] = '\0'; // temporarily make it \0.
	g_return_if_fail( is_int( &data[hold] ) );
	pic_sz = atoi( &data[hold] );
	data[i] = '\t'; // restore it to tab.

	++i;
	hold = i;
	for( ; i < sz && data[i] != '\t' ; ++i ); // find 4th tab. (shortcut)

	// extract shortcut information
	data[i] = '\0'; // temporarily make it \0.
	shortcut = g_strdup( &data[hold] );
	data[i] = '\t'; // restore

	// lastly, here comes binary data.
	++i;
	pic_data = (guchar*) &data[i];

	i += pic_sz;

	#if 0
	purple_debug_info("nateon", "[%s]\n"
		"\tshortcut: %s\n"
		"\tpic size: %ld\n", __FUNCTION__, shortcut, pic_sz );
	#endif

	nateon_conv_custom_smiley_add(swboard, shortcut, pic_data, pic_sz);

	// By now, our download is complete. Tell it to cmdproc
	emoticon_done_download(cmdproc);

	g_free(shortcut);
}

static void
quit_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSwitchBoard *swboard;
	const char *user;

	swboard = cmdproc->data;
	user = cmd->params[1];

	if (!(swboard->flag & NATEON_SB_FLAG_IM) && (swboard->conv != NULL))
		purple_debug_error("nateon_switchboard", "bye_cmd: helper bug\n");

	if (swboard->conv == NULL)
	{
		/* This is a helper switchboard */
		nateon_switchboard_destroy(swboard);
	}
	else if ((swboard->current_users > 1) ||
			 (purple_conversation_get_type(swboard->conv) == PURPLE_CONV_TYPE_CHAT))
	{
		/* This is a switchboard used for a chat */
		purple_conv_chat_remove_user(PURPLE_CONV_CHAT(swboard->conv), user, NULL);
		swboard->current_users--;
		if (swboard->current_users == 0)
			nateon_switchboard_destroy(swboard);
	}
	else
	{
		/* This is a switchboard used for a im session */
		nateon_switchboard_destroy(swboard);
	}
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

static void
join_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session;
	PurpleAccount *account;
	PurpleConnection *gc;
	NateonSwitchBoard *swboard;
	const char *account_name;

	account_name = cmd->params[1];

	session = cmdproc->session;
	account = session->account;
	gc = account->gc;
	swboard = cmdproc->data;

	nateon_switchboard_add_user(swboard, account_name);

	process_queue(swboard);

	nateon_cmdproc_process_txqueue(swboard->cmdproc);

//	if (!session->http_method)
//		send_clientcaps(swboard);

	if (swboard->closed)
		nateon_switchboard_close(swboard);
}

static void
whsp_emoticon_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	char **split = g_strsplit(cmd->params[3], "%09", 0);
	// const char *account_name = cmd->params[1];
	NateonSession *session = cmdproc->session;
	// NateonSwitchBoard *swboard = cmdproc->data;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	if (!purple_account_get_bool(session->account, "custom_smileys", TRUE))
		return;

	// It is fine to use strcmp (not strncmp)
	// when not both of the params are variables.
	// e.g strcmp(var1,"const") or strcmp("const",var1).
	// not file when strcmp(var1,var2)

	if( !strcmp(split[0], "REQUEST2") )
	{
		// Peer want to send me emoticons:
		// REQUEST2 = split[0]
		// 2 = emoticon request count = split[1]
			// 헣 shortcut1
			// 킄 shortcut2
		// 2 ??? always 2

		// I confirm the request with ACK2 like this:
		// WHSP 0
		// ssss@nate.com|dpc_03805:12876|X0HA
		// EMOTICON ACK2%09
		// 2%09 (cnt)
			// NCE20110218225301.bmp%09 (filename1)
			// 1498%09 (sz1)
			// :_%09 (shortcut1)
			// NCE20110218225251.bmp%09 (filename2)
			// 1498%09 (sz2)
			// ;-%09 (shortcut2)
		// received emoticon request.

		int i;
		int emo_cnt = atoi(split[1]);
		NateonTransaction *trans;
		GString *list = g_string_new("");
		char *who = no_dp(cmd->params[1]); // peer id, without dpkey.

		for (i = 0; i < emo_cnt; i++)
		{
			const char *shortcut = split[2+i];
			PurpleSmiley *ps = purple_smileys_find_by_shortcut(shortcut);
			PurpleStoredImage *img = purple_smiley_get_stored_image(ps);

			g_string_append_printf(list, "%s%%09%ld%%09%s%%09",
				purple_imgstore_get_filename(img),
				purple_imgstore_get_size(img),
				shortcut );

			purple_imgstore_unref(img);
		}

		trans = nateon_transaction_new(
			cmdproc, "WHSP", "%s EMOTICON ACK2%%09%d%%09%s",
			who, emo_cnt, list->str );
		nateon_cmdproc_send_trans(cmdproc, trans);

		g_free(who);
		g_string_free(list,TRUE);
	}
	else if( !strcmp(split[0], "ACK2" ) )
	{
		// Our will to send emoticon (EMOTICON REQUEST) has been
		// ack'ed. We say that we are ready to receive data in response.

		// input: ACK2%09 [0]
		// 2%09 (cnt) [1]
		// ec785ae80a50a5e2d5aa7164043e0c17d2a96785.png%09 (fname1) [2]
		// 3815%09 (sz1) [3]
		// 헣%09 (shortcut1) [4]
		// 0e6ef82641976f9d4860f01b235bb653c66afcd0.png%09 (fname2) [5]
		// 3639%09 (sz2) [6]
		// 킄%09 (shortcut2) [7]

		// output:
		// WHSP 0 
		// ssss@nate.com
		// EMOTICON REQDATA%09
		// 2%09 (cnt)
		// 헣%09 (sc1)
		// 킄%09 (sc2)
		int i;
		int emo_cnt = atoi(split[1]);
		NateonTransaction *trans;
		GString *list = g_string_new("");
		char *who = no_dp(cmd->params[1]); // peer id, without dpkey.

		for (i = 0; i < emo_cnt; i++)
		{
			const char *shortcut = split[3*i+4];
			g_string_append_printf(list, "%s%%09", shortcut);
		}

		trans = nateon_transaction_new(
			cmdproc, "WHSP", "%s EMOTICON REQDATA%%09%d%%09%s",
			who, emo_cnt, list->str );
		nateon_transaction_set_timeout_cb(trans, emoticon_timeout);
		nateon_cmdproc_send_trans(cmdproc, trans);

		g_free(who);
		g_string_free(list,TRUE);
	}
	else if( !strcmp(split[0], "REQDATA" ) )
	{
		// input:
		// REQDATA [0]
		// 1 [1] (cnt)
		// 헣 [2] (shortcut1)
		// 킄 [3] (shortcut2)

		// As of Windows version of NateOn 4.x, there is a bug
		// (or it might be a feature!) which only sends data
		// of one  emoticon...
		// We'll do that too and only send image of shortcut1.
		// Our response will be...
		// CTOC who U len
		// EMFL filename filesz shortcut [binarydata].
		size_t emfl_sz;
		NateonTransaction *trans;
		char  *emfl;
		char *who = no_dp(cmd->params[1]); // peer id, without dpkey.
		
		emfl = make_emfl(&emfl_sz, split[2]);

		trans = nateon_transaction_new(cmdproc, "CTOC", "%s U %ld", who, emfl_sz );
		nateon_transaction_set_payload(trans, emfl, emfl_sz);

		nateon_cmdproc_send_trans(cmdproc, trans);
		g_free(who);
		g_free(emfl);
	}
	else
	{
		purple_debug_info("nateon",
			"[%s] Unhandled command:\n"
			"\t%s\n", __FUNCTION__, split[0]);
	}

	g_strfreev(split);
}

static void
file_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	char **split = g_strsplit(cmd->params[3], "%09", 0);
	const char *account_name = cmd->params[1];
	NateonSession *session = cmdproc->session;
	NateonSwitchBoard *swboard = cmdproc->data;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	// It is fine to use strcmp (not strncmp)
	// when not both of the params are variables.
	// e.g strcmp(var1,"const") or strcmp("const",var1).
	// not file when strcmp(var1,var2)

	if( !strcmp(split[0], "REQUEST") )
	{
		/* Receiving File Transfer */
		int i;
		int num_files = atoi(split[1]);
		for (i = 0; i < num_files; i++) {
			char **file_data;
			char *filename;

			file_data = g_strsplit(split[i+2], "|", 0);	
			filename = purple_strreplace(file_data[0], "%20", " ");

			nateon_xfer_receive_file(session, swboard, account_name,
				filename, atoi(file_data[1]), file_data[2]);

			g_free(filename);
			g_strfreev(file_data);
		}
	}
	else if( !strcmp(split[0], "NACK" ) )
	{
		/* Your friend rejected your file transfer */
		// xxx@nate.com|dpc_01003:28058|bwzc
		// FILE NACK%091%09harry_16k.wav|420262|224:10026452103:448
		int i;
		int num_files = atoi(split[1]);

		for (i = 0; i < num_files; i++) {
			char **file_data;
			char *filename;

			file_data = g_strsplit(split[i+2], "|", 0);	
			filename = purple_strreplace(file_data[0], "%20", " ");

			nateon_xfer_request_denied(session, account_name, filename, file_data[2]);

			g_free(filename);
			g_strfreev(file_data);
		}
	}
	else if ( !strcmp(split[0], "CANCEL") )
	{
		/* Your friend canceled transfer */
		nateon_xfer_cancel_transfer(session, account_name, NULL, split[2]);
	}
	else if ( !strcmp(split[0], "ACK") )
	{
		// do nothing?
		// we DO get CTOC REQC NEW anyway, coming with ACK.
	}
	else
	{
		purple_debug_info("nateon", "[%s] You must not see this!\n", __FUNCTION__);
	}

	g_strfreev(split);
}

static void
whsp_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	// It is possible that we get WHSP 2 or something like that;
	// no command what-so-ever.
	if( cmd->param_count == 0 )
		return;
	if( cmd->param_count == 1 && is_int(cmd->params[0]) )
		return;

	// FILE transfer commands
	if ( cmd->param_count == 4 && !strcmp(cmd->params[2], "FILE") )
	{
		file_cmd( cmdproc, cmd );
	}
	else if (!strcmp(cmd->params[2], "AVCHAT2")) {} // ignore this cmd
	else if (!strcmp(cmd->params[2], "MAIL")) {} // ignore this cmd
	else if (!strcmp(cmd->params[2], "FONT")) {} // ignore this cmd
	else if (cmd->param_count == 4 && !strcmp(cmd->params[2], "EMOTICON"))
	{
		whsp_emoticon_cmd( cmdproc, cmd );
	}
	else if (cmd->param_count == 4 && !strcmp(cmd->params[2], "DPIMG") && \
			!strncmp(cmd->params[3], "REQUEST", strlen("REQUEST")) )
	{
		/* Receive Buddy Image */
		char **split;
		char **file_data;
		PurpleBuddyIcon *icon;
		// char *icon_checksum;
		NateonSession *session;
		NateonSwitchBoard *swboard;
		const char *account_name;

		session = cmdproc->session;
		swboard = cmdproc->data;

		account_name = cmd->params[1];

		split = g_strsplit(cmd->params[3], "%09", 0);
		file_data = g_strsplit(split[2], "|", 0);

		icon = purple_buddy_icons_find(session->account, account_name);
		if ( (icon == NULL) || \
				( strcmp(purple_buddy_icon_get_checksum(icon), file_data[0]) != 0))
		{
			nateon_xfer_receive_buddyimage(session, swboard, account_name, file_data[0],\
					atoi(file_data[1]), file_data[2]);
		}
		if (icon)
			purple_buddy_icon_unref(icon);

		g_strfreev(split);
	}
	else
	{
		int i;
		purple_debug_info("nateon", "[%s] Unhandled command...\n", __FUNCTION__);
		for( i = 0 ; i < cmd->param_count ; ++i )
		{
			purple_debug_info("nateon", "[%s] par%d: %s\n",
				__FUNCTION__, i, cmd->params[i] );
		}
	}
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
	char *body_enc;
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

	// when emoticon is being downloaded, postpone showing it.
	if( cmdproc->emoticon_download_cnt > 0 )
	{
		purple_debug_info("nateon",
			"[%s] %d emoticons are being downloaded, queueing instead.\n",
			__FUNCTION__, cmdproc->emoticon_download_cnt);
		nateon_command_ref(cmd); // add reference count that this cmd will not be freed
			// until dequeued from rx queue.
		nateon_cmdproc_queue_rx(cmdproc, cmd);
		return;
	}

	account_name = cmd->params[1];
	body_str = cmd->params[3];

//	{
//		char **params;
//
//		params = g_strsplit(body_str, "%09", 0);
//
//		body_final = g_strdup(purple_url_decode(params[3]));
//	}
	body_enc = g_markup_escape_text(body_str, -1);
	body_final = nateon_parse_format(body_enc);

	purple_debug_info("nateon", "%s - %s %s\n", __FUNCTION__, account_name, body_final);

	// Now we expect our downloads to be complete.
	// Scan for emoticons in message and draw it
	if (purple_account_get_bool(cmdproc->session->account, "custom_smileys", TRUE))
		nateon_conv_custom_smiley_draw(cmdproc, body_final);

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
	{
//		char *pre, *post;

//		nateon_parse_format(value, &pre, &post);

//		body_final = g_strdup_printf("%s%s%s", pre ? pre : "",
//									 body_enc ? body_enc : "", post ? post : "");
//
//		g_free(pre);
//		g_free(post);
//		g_free(body_enc);
	}
//	else
//	{
//		body_final = body_enc;
//	}

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

/**
 * handle EMOTICON subcommand in MESG command.
 */
static void
mesg_emoticon_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd, const char *who)
{
	const char *args = cmd->params[3];
	char **split = g_strsplit(args, "%09", 0);
	NateonSwitchBoard *swboard = cmdproc->data;

	if( !strcmp(split[0], "OBJECT") )
	{
		// input:
		// OBJECT [0] this subcommand.
		// 0000001 [1] ??? always this.
		// 2 [2], # emoticons
			// NCE20110218225246.bmp [fname1]
			// +_+ [shortcut1]
			// NCE20110218225251.bmp [fname2]
			// ;-%09 [shortcut2]
		// 2 [last], always 2.
		
		// output:
		// send this whisper back, because we want to receive the actual emoticon.
		// WHSP 0 ssss@nate.com|dpc_06401:29694|X0HA EMOTICON REQUEST2%092%09+_+%09;-%092
		// WHSP 0 ssss@nate.com|dpc_01202:26225|bEOi EMOTICON REQUEST2%091%09;-%092
		GString *list;
		NateonTransaction *trans;
		int cnt = atoi(split[2]);
		int i;
		int req_cnt = 0; // # of not downloaded emoticons

		if( cnt >= 1 ) // we sometimes get cnt == 0, too.
		{
			list = g_string_new("");
			for( i = 0 ; i < cnt ; ++i )
			{
				const char *shortcut = split[2*i+4];
				if( !nateon_conv_custom_smiley_find(swboard, shortcut) )
				{
					// If smiley is not available, make request for it.
					++req_cnt;
					g_string_append_printf(list, "%s%%09", shortcut);
				}
			}

			if( req_cnt > 0 )
			{
				trans = nateon_transaction_new(
					cmdproc, "WHSP", "%s EMOTICON REQUEST2%%09%d%%09%s2",
					who, req_cnt, list->str );
				nateon_transaction_set_timeout_cb(trans, emoticon_timeout);
				cmdproc->emoticon_download_cnt++; // Turn on download flag.
				// Note that we only add 1, due to nateon 4.x bug, which
				// would only transmit us only one emoticon even though we made
				// multiple request with req_cnt.
				// Ideally, it should be:
				// cmdproc->emoticon_download_cnt += req_cnt;
				nateon_cmdproc_send_trans(cmdproc, trans);
			}

			g_string_free(list,TRUE);
		}
	}

	g_strfreev(split);
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
		if (!strcmp(cmd->params[3], "1"))
		{
			serv_got_typing(gc, swboard->im_user, NATEON_TYPING_RECV_TIMEOUT, PURPLE_TYPING);
		}
		else
		{
			serv_got_typing_stopped(gc, swboard->im_user);
		}
	}
	else if (!strcmp(subcmd, "EMOTICON"))
	{
		if (!purple_account_get_bool(cmdproc->session->account, "custom_smileys", TRUE))
			return;
		swboard = cmdproc->data;
		mesg_emoticon_cmd(cmdproc, cmd, swboard->im_user);
	}
	else
	{
		purple_debug_info("nateon", "[%s] Unhandled command %s\n",
			__FUNCTION__, subcmd);
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

	purple_debug_info("nateon", "[%s:%d]\n", __FUNCTION__, __LINE__);

	swboard = cmdproc->data;

	// 수정필요 =.=/
	swboard->total_users = atoi(cmd->params[2]);
	nateon_switchboard_add_user(swboard, cmd->params[3]);

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
	nateon_cmdproc_process_txqueue(cmdproc);
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
static void entr_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error);

static void
connect_cb(NateonServConn *servconn)
{
	NateonSwitchBoard *swboard;
	NateonTransaction *trans;
	NateonCmdProc *cmdproc;
	PurpleAccount *account;
	NateonUser *user;

	purple_debug_info("nateon", "switchboard - %s\n", __FUNCTION__);

	cmdproc = servconn->cmdproc;
	g_return_if_fail(cmdproc != NULL);

	account = cmdproc->session->account;
	swboard = cmdproc->data;
	g_return_if_fail(swboard != NULL);

	user = cmdproc->session->user;

	if (nateon_switchboard_is_invited(swboard))
	{
		purple_debug_info("nateon", "== invited ==\n");
		swboard->empty = FALSE;
	}
	trans = nateon_transaction_new(cmdproc,
		"ENTR", "%s|%s %s %s %s UTF8 P 3.0 1",
			purple_account_get_username(account),
			cmdproc->session->dpkey,
			purple_strreplace(user->store_name, " ", "%20"),
			user->friendly_name,
			swboard->auth_key);

	nateon_transaction_set_error_cb(trans, entr_error);
	nateon_transaction_set_data(trans, swboard);
	nateon_cmdproc_send_trans(cmdproc, trans);
}

static void
entr_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
{
	NateonSwitchBoard *swboard;
	char **params;
	char *account;
//	int reason = MSN_SB_ERROR_UNKNOWN;

	purple_debug_info("nateon", "[%s] error(%d)\n", __FUNCTION__, error);

//	if (error == 911)
//	{
//		reason = MSN_SB_ERROR_AUTHFAILED;
//	}

	purple_debug_warning("nateon", "nateon_error: command %s gave error %i\n", trans->command, error);

	params = g_strsplit(trans->params, " ", 0);
	account = params[0];
	swboard = trans->data;

	swboard_error_helper(swboard, NATEON_SB_ERROR_UNKNOWN, account);

	g_strfreev(params);
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
 * Invite stuff
 **************************************************************************/
static void
got_invt(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
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
}

static void
invt_timeout(NateonCmdProc *cmdproc, NateonTransaction *trans)
{
	purple_debug_warning("nateon", "invt_timeout: command %s timed out\n", trans->command);

	invt_error_helper(trans, NATEON_SB_ERROR_UNKNOWN);
}

/**
 * When emoticon download request is timed out...
 * It is possible to get packet loss and I'm prepared to handle it!
 */
static void
emoticon_timeout(NateonCmdProc *cmdproc, NateonTransaction *trans)
{
	purple_debug_warning("nateon", "Emoticon data request timeout");

	emoticon_done_download(cmdproc);

	invt_error_helper(trans, NATEON_SB_ERROR_UNKNOWN);
}

static void
invt_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
{
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
}

void
nateon_switchboard_request_add_user(NateonSwitchBoard *swboard, const char *user)
{
	NateonCmdProc *cmdproc;
	NateonTransaction *trans;
	NateonServConn *servconn;
	PurpleAccount *account;
	const char *username;
	char *payload;
	size_t payload_len;

	g_return_if_fail(swboard != NULL);

	cmdproc = swboard->session->notification->cmdproc;
	account = cmdproc->session->account;
	servconn = swboard->servconn;

	username = purple_account_get_username(account);

	payload = g_strdup_printf("INVT %s %s 5004 %s", username, servconn->host, swboard->auth_key);
	payload_len = strlen(payload);

	trans = nateon_transaction_new(cmdproc, "CTOC", "%s A %d", user, payload_len);
	/* this doesn't do anything, but users seem to think that
	 * 'Unhandled command' is some kind of error, so we don't report it */
	nateon_transaction_add_cb(trans, "PACK", got_invt);

	nateon_transaction_set_payload(trans, payload, payload_len);
//	nateon_transaction_set_data(trans, swboard);
	nateon_transaction_set_timeout_cb(trans, invt_timeout);

	g_free(payload);

	if (swboard->ready)
	{
		purple_debug_info("nateon", "[%s] send_trans\n", __FUNCTION__);
		nateon_cmdproc_send_trans(cmdproc, trans);
	}
	else
	{
		purple_debug_info("nateon", "[%s] queue_trans\n", __FUNCTION__);
		nateon_cmdproc_queue_tx(cmdproc, trans);
	}
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

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	swboard = cmd->trans->data;

	if (g_list_find(cmdproc->session->switches, swboard) == NULL)
		/* The conversation window was closed. */
		return;

	nateon_switchboard_set_auth_key(swboard, cmd->params[3]);

	host = g_strdup(cmd->params[1]);
	port = atoi(cmd->params[2]);

	if (!nateon_switchboard_connect(swboard, host, port))
	{
		purple_debug_info("nateon", "[%s] connection failed\n", __FUNCTION__);
		nateon_switchboard_destroy(swboard);
	}
	else
	{
		purple_debug_info("nateon", "[%s] connection success\n", __FUNCTION__);
		nateon_switchboard_request_add_user(swboard, swboard->im_user);
	}
//	{
//		NateonCmdProc *cmdproc;
//		NateonSession *session = swboard->session;
//		NateonServConn *servconn;
//		PurpleAccount *account = session->account;
//		const char *username;
//		NateonUser *user;
//
//		NateonTransaction *trans;
//		char *payload;
//		size_t payload_len;
//
//		cmdproc = swboard->session->notification->cmdproc;
//		servconn = cmdproc->servconn;
//		
//		user = nateon_userlist_find_user_with_name(session->userlist, swboard->im_user);
//		username = purple_account_get_username(account);
//
//		payload = g_strdup_printf("INVT %s %s %d %s", username, host, port, swboard->auth_key);
//		payload_len = strlen(payload);
//
//		purple_debug_info("nateon", "[%s] payload(%s)\n", __FUNCTION__, payload);
//
//		purple_debug_info("nateon", "[%s] CTOC에서 A가 들어가야하나? ㅡ.ㅡa\n", __FUNCTION__);
//
////		trans = nateon_transaction_new(cmdproc, "CTOC", "%s %s %d", swboard->im_user, user->status, payload_len);
//		trans = nateon_transaction_new(cmdproc, "CTOC", "%s A %d", swboard->im_user, payload_len);
////		nateon_transaction_add_cb(trans, "PACK", got_invt);
//
//		nateon_transaction_set_payload(trans, payload, payload_len);
////		nateon_transaction_set_data(trans, swboard);
////		nateon_transaction_set_timeout_cb(trans, cal_timeout);
//
//		g_free(payload);
//
//		nateon_cmdproc_send_trans(cmdproc, trans);
//	}

	g_free(host);
}

static void
ress_error(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
{
//	NateonSwitchBoard *swboard;
//	int reason = NATEON_SB_ERROR_UNKNOWN;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);
	purple_debug_info("nateon", "[%s] error(%d)\n", __FUNCTION__, error);

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
}

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
	nateon_transaction_set_error_cb(trans, ress_error);

	nateon_cmdproc_send_trans(cmdproc, trans);
}

void
nateon_switchboard_close(NateonSwitchBoard *swboard)
{
	g_return_if_fail(swboard != NULL);

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	if (swboard->error != NATEON_SB_ERROR_NONE)
	{
		nateon_switchboard_destroy(swboard);
	}
	else if (g_queue_is_empty(swboard->msg_queue) ||
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
 * PRS  
**************************************************************************/ 
static void rcon_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd) 
{ 
	connect_cb(cmdproc->servconn); 
} 

/**************************************************************************
 * Init stuff
 **************************************************************************/

void
nateon_switchboard_init(void)
{
	cbs_table = nateon_table_new();

	nateon_table_add_cmd(cbs_table, "RCON", "RCON", rcon_cmd);

	nateon_table_add_cmd(cbs_table, "ENTR", "ENTR", entr_cmd);
//	nateon_table_add_cmd(cbs_table, "ANS", "ANS", ans_cmd);
//	nateon_table_add_cmd(cbs_table, "ANS", "IRO", iro_cmd);
//
//	nateon_table_add_cmd(cbs_table, "MSG", "ACK", ack_cmd);
//	nateon_table_add_cmd(cbs_table, "MSG", "NAK", nak_cmd);
//
//	nateon_table_add_cmd(cbs_table, "USR", "USR", usr_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "USER", user_cmd);

//	nateon_table_add_cmd(cbs_table, NULL, "MSG", msg_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "MESG", mesg_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "JOI", joi_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "JOIN", join_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "BYE", bye_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "OUT", out_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "WHSP", whsp_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "QUIT", quit_cmd);

	nateon_table_add_cmd(cbs_table, NULL, "CTOC", swb_ctoc_cmd);
	nateon_table_add_cmd(cbs_table, NULL, "EMFL", swb_emfl_cmd);
//
//#if 0
//	/* They might skip the history */
//	nateon_table_add_cmd(cbs_table, NULL, "ACK", NULL);
//#endif

//	nateon_table_add_error(cbs_table, "MESG", mesg_error);
	nateon_table_add_error(cbs_table, "INVT", invt_error);
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

/* vim: ts=4 sw=4: */
