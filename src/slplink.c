/**
 * @file slplink.c NATEONSLP Link support
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
#include "slplink.h"

#include "switchboard.h"
//#include "slp.h"

//void nateon_slplink_send_msgpart(NateonSlpLink *slplink, NateonSlpMessage *slpmsg);

//#ifdef NATEON_DEBUG_SLP_FILES
//static int m_sc = 0;
//static int m_rc = 0;
//
//static void
//debug_msg_to_file(NateonMessage *msg, gboolean send)
//{
//	char *tmp;
//	char *dir;
//	char *pload;
//	FILE *tf;
//	int c;
//	gsize pload_size;
//
//	dir = send ? "send" : "recv";
//	c = send ? m_sc++ : m_rc++;
//	tmp = g_strdup_printf("%s/nateontest/%s/%03d", g_get_home_dir(), dir, c);
//	tf = g_fopen(tmp, "wb");
//	if (tf == NULL)
//	{
//		purple_debug_error("nateon", "could not open debug file");
//		return;
//	}
//	pload = nateon_message_gen_payload(msg, &pload_size);
//	fwrite(pload, 1, pload_size, tf);
//	fclose(tf);
//	g_free(tmp);
//}
//#endif

/**************************************************************************
 * Main
 **************************************************************************/

//NateonSlpLink *
//nateon_slplink_new(NateonSession *session, const char *username)
//{
//	NateonSlpLink *slplink;
//
//	g_return_val_if_fail(session != NULL, NULL);
//
//	slplink = g_new0(NateonSlpLink, 1);
//
//#ifdef NATEON_DEBUG_SLPLINK
//	purple_debug_info("nateon", "slplink_new: slplink(%p)\n", slplink);
//#endif
//
//	slplink->session = session;
//	slplink->slp_seq_id = rand() % 0xFFFFFF00 + 4;
//
//	slplink->local_user = g_strdup(nateon_user_get_passport(session->user));
//	slplink->remote_user = g_strdup(username);
//
//	slplink->slp_msg_queue = g_queue_new();
//
//	session->slplinks =
//		g_list_append(session->slplinks, slplink);
//
//	return slplink;
//}
//
//void
//nateon_slplink_destroy(NateonSlpLink *slplink)
//{
//	NateonSession *session;
//
//#ifdef NATEON_DEBUG_SLPLINK
//	purple_debug_info("nateon", "slplink_destroy: slplink(%p)\n", slplink);
//#endif
//
//	g_return_if_fail(slplink != NULL);
//
//	if (slplink->swboard != NULL)
//		slplink->swboard->slplinks = g_list_remove(slplink->swboard->slplinks, slplink);
//
//	session = slplink->session;
//
//	if (slplink->local_user != NULL)
//		g_free(slplink->local_user);
//
//	if (slplink->remote_user != NULL)
//		g_free(slplink->remote_user);
//
//	if (slplink->directconn != NULL)
//		nateon_directconn_destroy(slplink->directconn);
//
//	while (slplink->slp_calls != NULL)
//		nateon_slp_call_destroy(slplink->slp_calls->data);
//
//	session->slplinks =
//		g_list_remove(session->slplinks, slplink);
//
//	g_free(slplink);
//}
//
NateonSlpLink *
nateon_session_find_slplink(NateonSession *session, const char *who)
{
	GList *l;

	for (l = session->slplinks; l != NULL; l = l->next)
	{
		NateonSlpLink *slplink;

		slplink = l->data;

		if (!strcmp(slplink->remote_user, who))
			return slplink;
	}

	return NULL;
}

NateonSlpLink *
nateon_session_get_slplink(NateonSession *session, const char *username)
{
	NateonSlpLink *slplink;

	g_return_val_if_fail(session != NULL, NULL);
	g_return_val_if_fail(username != NULL, NULL);

	slplink = nateon_session_find_slplink(session, username);

	if (slplink == NULL)
		slplink = nateon_slplink_new(session, username);

	return slplink;
}

//NateonSlpSession *
//nateon_slplink_find_slp_session(NateonSlpLink *slplink, long session_id)
//{
//	GList *l;
//	NateonSlpSession *slpsession;
//
//	for (l = slplink->slp_sessions; l != NULL; l = l->next)
//	{
//		slpsession = l->data;
//
//		if (slpsession->id == session_id)
//			return slpsession;
//	}
//
//	return NULL;
//}
//
//void
//nateon_slplink_add_slpcall(NateonSlpLink *slplink, NateonSlpCall *slpcall)
//{
//	if (slplink->swboard != NULL)
//		slplink->swboard->flag |= NATEON_SB_FLAG_FT;
//
//	slplink->slp_calls = g_list_append(slplink->slp_calls, slpcall);
//}
//
//void
//nateon_slplink_remove_slpcall(NateonSlpLink *slplink, NateonSlpCall *slpcall)
//{
//	slplink->slp_calls = g_list_remove(slplink->slp_calls, slpcall);
//
//	/* The slplink has no slpcalls in it. If no one is using it, we might
//	 * destroy the switchboard, but we should be careful not to use the slplink
//	 * again. */
//	if (slplink->slp_calls == NULL)
//	{
//		if (slplink->swboard != NULL)
//		{
//			if (nateon_switchboard_release(slplink->swboard, NATEON_SB_FLAG_FT))
//				/* I'm not sure this is the best thing to do, but it's better
//				 * than nothing. */
//				slpcall->slplink = NULL;
//		}
//	}
//}
//
//NateonSlpCall *
//nateon_slplink_find_slp_call(NateonSlpLink *slplink, const char *id)
//{
//	GList *l;
//	NateonSlpCall *slpcall;
//
//	if (!id)
//		return NULL;
//
//	for (l = slplink->slp_calls; l != NULL; l = l->next)
//	{
//		slpcall = l->data;
//
//		if (slpcall->id && !strcmp(slpcall->id, id))
//			return slpcall;
//	}
//
//	return NULL;
//}
//
//NateonSlpCall *
//nateon_slplink_find_slp_call_with_session_id(NateonSlpLink *slplink, long id)
//{
//	GList *l;
//	NateonSlpCall *slpcall;
//
//	for (l = slplink->slp_calls; l != NULL; l = l->next)
//	{
//		slpcall = l->data;
//
//		if (slpcall->session_id == id)
//			return slpcall;
//	}
//
//	return NULL;
//}
//
//void
//nateon_slplink_send_msg(NateonSlpLink *slplink, NateonMessage *msg)
//{
//	if (slplink->directconn != NULL)
//	{
//		nateon_directconn_send_msg(slplink->directconn, msg);
//	}
//	else
//	{
//		if (slplink->swboard == NULL)
//		{
//			slplink->swboard = nateon_session_get_swboard(slplink->session,
//													   slplink->remote_user, NATEON_SB_FLAG_FT);
//
//			if (slplink->swboard == NULL)
//				return;
//
//			/* If swboard is destroyed we will be too */
//			slplink->swboard->slplinks = g_list_prepend(slplink->swboard->slplinks, slplink);
//		}
//
//		nateon_switchboard_send_msg(slplink->swboard, msg, TRUE);
//	}
//}
//
///* We have received the message ack */
//static void
//msg_ack(NateonMessage *msg, void *data)
//{
//	NateonSlpMessage *slpmsg;
//	long long real_size;
//
//	slpmsg = data;
//
//	real_size = (slpmsg->flags == 0x2) ? 0 : slpmsg->size;
//
//	slpmsg->offset += msg->nateonslp_header.length;
//
//	if (slpmsg->offset < real_size)
//	{
//		nateon_slplink_send_msgpart(slpmsg->slplink, slpmsg);
//	}
//	else
//	{
//		/* The whole message has been sent */
//		if (slpmsg->flags == 0x20 || slpmsg->flags == 0x1000030)
//		{
//			if (slpmsg->slpcall != NULL)
//			{
//				if (slpmsg->slpcall->cb)
//					slpmsg->slpcall->cb(slpmsg->slpcall,
//						NULL, 0);
//			}
//		}
//	}
//
//	slpmsg->msgs = g_list_remove(slpmsg->msgs, msg);
//}
//
///* We have received the message nak. */
//static void
//msg_nak(NateonMessage *msg, void *data)
//{
//	NateonSlpMessage *slpmsg;
//
//	slpmsg = data;
//
//	nateon_slplink_send_msgpart(slpmsg->slplink, slpmsg);
//
//	slpmsg->msgs = g_list_remove(slpmsg->msgs, msg);
//}
//
//void
//nateon_slplink_send_msgpart(NateonSlpLink *slplink, NateonSlpMessage *slpmsg)
//{
//	NateonMessage *msg;
//	long long real_size;
//	size_t len = 0;
//
//	/* Maybe we will want to create a new msg for this slpmsg instead of
//	 * reusing the same one all the time. */
//	msg = slpmsg->msg;
//
//	real_size = (slpmsg->flags == 0x2) ? 0 : slpmsg->size;
//
//	if (slpmsg->offset < real_size)
//	{
//		if (slpmsg->fp)
//		{
//			char data[1202];
//			len = fread(data, 1, sizeof(data), slpmsg->fp);
//			nateon_message_set_bin_data(msg, data, len);
//		}
//		else
//		{
//			len = slpmsg->size - slpmsg->offset;
//
//			if (len > 1202)
//				len = 1202;
//
//			nateon_message_set_bin_data(msg, slpmsg->buffer + slpmsg->offset, len);
//		}
//
//		msg->nateonslp_header.offset = slpmsg->offset;
//		msg->nateonslp_header.length = len;
//	}
//
//#ifdef NATEON_DEBUG_SLP
//	nateon_message_show_readable(msg, slpmsg->info, slpmsg->text_body);
//#endif
//
//#ifdef NATEON_DEBUG_SLP_FILES
//	debug_msg_to_file(msg, TRUE);
//#endif
//
//	slpmsg->msgs =
//		g_list_append(slpmsg->msgs, msg);
//	nateon_slplink_send_msg(slplink, msg);
//
//	if ((slpmsg->flags == 0x20 || slpmsg->flags == 0x1000030) &&
//		(slpmsg->slpcall != NULL))
//	{
//		slpmsg->slpcall->progress = TRUE;
//
//		if (slpmsg->slpcall->progress_cb != NULL)
//		{
//			slpmsg->slpcall->progress_cb(slpmsg->slpcall, slpmsg->size,
//										 len, slpmsg->offset);
//		}
//	}
//
//	/* slpmsg->offset += len; */
//}
//
//void
//nateon_slplink_release_slpmsg(NateonSlpLink *slplink, NateonSlpMessage *slpmsg)
//{
//	NateonMessage *msg;
//
//	slpmsg->msg = msg = nateon_message_new_nateonslp();
//
//	if (slpmsg->flags == 0x0)
//	{
//		msg->nateonslp_header.session_id = slpmsg->session_id;
//		msg->nateonslp_header.ack_id = rand() % 0xFFFFFF00;
//	}
//	else if (slpmsg->flags == 0x2)
//	{
//		msg->nateonslp_header.session_id = slpmsg->session_id;
//		msg->nateonslp_header.ack_id = slpmsg->ack_id;
//		msg->nateonslp_header.ack_size = slpmsg->ack_size;
//		msg->nateonslp_header.ack_sub_id = slpmsg->ack_sub_id;
//	}
//	else if (slpmsg->flags == 0x20 || slpmsg->flags == 0x1000030)
//	{
//		NateonSlpSession *slpsession;
//		slpsession = slpmsg->slpsession;
//
//		g_return_if_fail(slpsession != NULL);
//		msg->nateonslp_header.session_id = slpsession->id;
//		msg->nateonslp_footer.value = slpsession->app_id;
//		msg->nateonslp_header.ack_id = rand() % 0xFFFFFF00;
//	}
//	else if (slpmsg->flags == 0x100)
//	{
//		msg->nateonslp_header.ack_id     = slpmsg->ack_id;
//		msg->nateonslp_header.ack_sub_id = slpmsg->ack_sub_id;
//		msg->nateonslp_header.ack_size   = slpmsg->ack_size;
//	}
//
//	msg->nateonslp_header.id = slpmsg->id;
//	msg->nateonslp_header.flags = slpmsg->flags;
//
//	msg->nateonslp_header.total_size = slpmsg->size;
//
//	nateon_message_set_attr(msg, "P2P-Dest", slplink->remote_user);
//
//	msg->ack_cb = msg_ack;
//	msg->nak_cb = msg_nak;
//	msg->ack_data = slpmsg;
//
//	nateon_slplink_send_msgpart(slplink, slpmsg);
//
//	nateon_message_destroy(msg);
//}
//
//void
//nateon_slplink_queue_slpmsg(NateonSlpLink *slplink, NateonSlpMessage *slpmsg)
//{
//	slpmsg->id = slplink->slp_seq_id++;
//
//	g_queue_push_head(slplink->slp_msg_queue, slpmsg);
//}
//
//void
//nateon_slplink_send_slpmsg(NateonSlpLink *slplink, NateonSlpMessage *slpmsg)
//{
//	slpmsg->id = slplink->slp_seq_id++;
//
//	nateon_slplink_release_slpmsg(slplink, slpmsg);
//}
//
//void
//nateon_slplink_unleash(NateonSlpLink *slplink)
//{
//	NateonSlpMessage *slpmsg;
//
//	/* Send the queued msgs in the order they came. */
//
//	while ((slpmsg = g_queue_pop_tail(slplink->slp_msg_queue)) != NULL)
//	{
//		nateon_slplink_release_slpmsg(slplink, slpmsg);
//	}
//}
//
//void
//nateon_slplink_send_ack(NateonSlpLink *slplink, NateonMessage *msg)
//{
//	NateonSlpMessage *slpmsg;
//
//	slpmsg = nateon_slpmsg_new(slplink);
//
//	slpmsg->session_id = msg->nateonslp_header.session_id;
//	slpmsg->size       = msg->nateonslp_header.total_size;
//	slpmsg->flags      = 0x02;
//	slpmsg->ack_id     = msg->nateonslp_header.id;
//	slpmsg->ack_sub_id = msg->nateonslp_header.ack_id;
//	slpmsg->ack_size   = msg->nateonslp_header.total_size;
//
//#ifdef NATEON_DEBUG_SLP
//	slpmsg->info = "SLP ACK";
//#endif
//
//	nateon_slplink_send_slpmsg(slplink, slpmsg);
//}
//
//static void
//send_file_cb(NateonSlpSession *slpsession)
//{
//	NateonSlpCall *slpcall;
//	NateonSlpMessage *slpmsg;
//	struct stat st;
//	PurpleXfer *xfer;
//
//	slpcall = slpsession->slpcall;
//	slpmsg = nateon_slpmsg_new(slpcall->slplink);
//	slpmsg->slpcall = slpcall;
//	slpmsg->flags = 0x1000030;
//	slpmsg->slpsession = slpsession;
//#ifdef NATEON_DEBUG_SLP
//	slpmsg->info = "SLP FILE";
//#endif
//	xfer = (PurpleXfer *)slpcall->xfer;
//	purple_xfer_start(slpcall->xfer, 0, NULL, 0);
//	slpmsg->fp = xfer->dest_fp;
//	if (g_stat(purple_xfer_get_local_filename(xfer), &st) == 0)
//		slpmsg->size = st.st_size;
//	xfer->dest_fp = NULL; /* Disable double fclose() */
//
//	nateon_slplink_send_slpmsg(slpcall->slplink, slpmsg);
//}
//
//void
//nateon_slplink_process_msg(NateonSlpLink *slplink, NateonMessage *msg)
//{
//	NateonSlpMessage *slpmsg;
//	const char *data;
//	gsize offset;
//	gsize len;
//
//#ifdef NATEON_DEBUG_SLP
//	nateon_slpmsg_show(msg);
//#endif
//
//#ifdef NATEON_DEBUG_SLP_FILES
//	debug_msg_to_file(msg, FALSE);
//#endif
//
//	if (msg->nateonslp_header.total_size < msg->nateonslp_header.length)
//	{
//		purple_debug_error("nateon", "This can't be good\n");
//		g_return_if_reached();
//	}
//
//	slpmsg = NULL;
//	data = nateon_message_get_bin_data(msg, &len);
//
//	/*
//		OVERHEAD!
//		if (msg->nateonslp_header.length < msg->nateonslp_header.total_size)
//	 */
//
//	offset = msg->nateonslp_header.offset;
//
//	if (offset == 0)
//	{
//		slpmsg = nateon_slpmsg_new(slplink);
//		slpmsg->id = msg->nateonslp_header.id;
//		slpmsg->session_id = msg->nateonslp_header.session_id;
//		slpmsg->size = msg->nateonslp_header.total_size;
//		slpmsg->flags = msg->nateonslp_header.flags;
//
//		if (slpmsg->session_id)
//		{
//			if (slpmsg->slpcall == NULL)
//				slpmsg->slpcall = nateon_slplink_find_slp_call_with_session_id(slplink, slpmsg->session_id);
//
//			if (slpmsg->slpcall != NULL)
//			{
//				if (slpmsg->flags == 0x20 || slpmsg->flags == 0x1000030)
//				{
//					PurpleXfer *xfer;
//
//					xfer = slpmsg->slpcall->xfer;
//
//					if (xfer != NULL)
//					{
//						purple_xfer_start(slpmsg->slpcall->xfer,
//							0, NULL, 0);
//						slpmsg->fp = ((PurpleXfer *)slpmsg->slpcall->xfer)->dest_fp;
//						xfer->dest_fp = NULL; /* Disable double fclose() */
//					}
//				}
//			}
//		}
//		if (!slpmsg->fp && slpmsg->size)
//		{
//			slpmsg->buffer = g_try_malloc(slpmsg->size);
//			if (slpmsg->buffer == NULL)
//			{
//				purple_debug_error("nateon", "Failed to allocate buffer for slpmsg\n");
//				return;
//			}
//		}
//	}
//	else
//	{
//		slpmsg = nateon_slplink_message_find(slplink, msg->nateonslp_header.session_id, msg->nateonslp_header.id);
//	}
//
//	if (slpmsg == NULL)
//	{
//		/* Probably the transfer was canceled */
//		purple_debug_error("nateon", "Couldn't find slpmsg\n");
//		return;
//	}
//
//	if (slpmsg->fp)
//	{
//		/* fseek(slpmsg->fp, offset, SEEK_SET); */
//		len = fwrite(data, 1, len, slpmsg->fp);
//	}
//	else if (slpmsg->size)
//	{
//		if ((offset + len) > slpmsg->size)
//		{
//			purple_debug_error("nateon", "Oversized slpmsg\n");
//			g_return_if_reached();
//		}
//		else
//			memcpy(slpmsg->buffer + offset, data, len);
//	}
//
//	if ((slpmsg->flags == 0x20 || slpmsg->flags == 0x1000030) &&
//		(slpmsg->slpcall != NULL))
//	{
//		slpmsg->slpcall->progress = TRUE;
//
//		if (slpmsg->slpcall->progress_cb != NULL)
//		{
//			slpmsg->slpcall->progress_cb(slpmsg->slpcall, slpmsg->size,
//										 len, offset);
//		}
//	}
//
//#if 0
//	if (slpmsg->buffer == NULL)
//		return;
//#endif
//
//	if (msg->nateonslp_header.offset + msg->nateonslp_header.length
//		>= msg->nateonslp_header.total_size)
//	{
//		/* All the pieces of the slpmsg have been received */
//		NateonSlpCall *slpcall;
//
//		slpcall = nateon_slp_process_msg(slplink, slpmsg);
//
//		if (slpmsg->flags == 0x100)
//		{
//			NateonDirectConn *directconn;
//
//			directconn = slplink->directconn;
//
//			if (!directconn->acked)
//				nateon_directconn_send_handshake(directconn);
//		}
//		else if (slpmsg->flags == 0x0 || slpmsg->flags == 0x20 ||
//				 slpmsg->flags == 0x1000030)
//		{
//			/* Release all the messages and send the ACK */
//
//			nateon_slplink_send_ack(slplink, msg);
//			nateon_slplink_unleash(slplink);
//		}
//
//		nateon_slpmsg_destroy(slpmsg);
//
//		if (slpcall != NULL && slpcall->wasted)
//			nateon_slp_call_destroy(slpcall);
//	}
//}
//
//NateonSlpMessage *
//nateon_slplink_message_find(NateonSlpLink *slplink, long session_id, long id)
//{
//	GList *e;
//
//	for (e = slplink->slp_msgs; e != NULL; e = e->next)
//	{
//		NateonSlpMessage *slpmsg = e->data;
//
//		if ((slpmsg->session_id == session_id) && (slpmsg->id == id))
//			return slpmsg;
//	}
//
//	return NULL;
//}
//
//typedef struct
//{
//	guint32 length;
//	guint32 unk1;
//	guint32 file_size;
//	guint32 unk2;
//	guint32 unk3;
//} NateonContextHeader;
//
//#define MAX_FILE_NAME_LEN 0x226
//
//static gchar *
//gen_context(const char *file_name, const char *file_path)
//{
//	struct stat st;
//	gsize size = 0;
//	NateonContextHeader header;
//	gchar *u8 = NULL;
//	guchar *base;
//	guchar *n;
//	gchar *ret;
//	gunichar2 *uni = NULL;
//	glong currentChar = 0;
//	glong uni_len = 0;
//	gsize len;
//
//	if (g_stat(file_path, &st) == 0)
//		size = st.st_size;
//
//	if(!file_name) {
//		u8 = purple_utf8_try_convert(g_basename(file_path));
//		file_name = u8;
//	}
//
//	uni = g_utf8_to_utf16(file_name, -1, NULL, &uni_len, NULL);
//
//	if(u8) {
//		g_free(u8);
//		file_name = NULL;
//		u8 = NULL;
//	}
//
//	len = sizeof(NateonContextHeader) + MAX_FILE_NAME_LEN + 4;
//
//	header.length = GUINT32_TO_LE(len);
//	header.unk1 = GUINT32_TO_LE(2);
//	header.file_size = GUINT32_TO_LE(size);
//	header.unk2 = GUINT32_TO_LE(0);
//	header.unk3 = GUINT32_TO_LE(0);
//
//	base = g_malloc(len + 1);
//	n = base;
//
//	memcpy(n, &header, sizeof(NateonContextHeader));
//	n += sizeof(NateonContextHeader);
//
//	memset(n, 0x00, MAX_FILE_NAME_LEN);
//	for(currentChar = 0; currentChar < uni_len; currentChar++) {
//		*((gunichar2 *)n + currentChar) = GUINT16_TO_LE(uni[currentChar]);
//	}
//	n += MAX_FILE_NAME_LEN;
//
//	memset(n, 0xFF, 4);
//	n += 4;
//
//	g_free(uni);
//	ret = purple_base64_encode(base, len);
//	g_free(base);
//	return ret;
//}
//
//void
//nateon_slplink_request_ft(NateonSlpLink *slplink, PurpleXfer *xfer)
//{
//	NateonSlpCall *slpcall;
//	char *context;
//	const char *fn;
//	const char *fp;
//
//	fn = purple_xfer_get_filename(xfer);
//	fp = purple_xfer_get_local_filename(xfer);
//
//	g_return_if_fail(slplink != NULL);
//	g_return_if_fail(fp != NULL);
//
//	slpcall = nateon_slp_call_new(slplink);
//	nateon_slp_call_init(slpcall, NATEON_SLPCALL_DC);
//
//	slpcall->session_init_cb = send_file_cb;
//	slpcall->end_cb = nateon_xfer_end_cb;
//	slpcall->progress_cb = nateon_xfer_progress_cb;
//	slpcall->cb = nateon_xfer_completed_cb;
//	slpcall->xfer = xfer;
//	purple_xfer_ref(slpcall->xfer);
//
//	slpcall->pending = TRUE;
//
//	purple_xfer_set_cancel_send_fnc(xfer, nateon_xfer_cancel);
//
//	xfer->data = slpcall;
//
//	context = gen_context(fn, fp);
//
//	nateon_slp_call_invite(slpcall, "5D3E02AB-6190-11D3-BBBB-00C04F795683", 2,
//						context);
//
//	g_free(context);
//}
//
//void
//nateon_slplink_request_object(NateonSlpLink *slplink,
//						   const char *info,
//						   NateonSlpCb cb,
//						   NateonSlpEndCb end_cb,
//						   const NateonObject *obj)
//{
//	NateonSlpCall *slpcall;
//	char *nateonobj_data;
//	char *nateonobj_base64;
//
//	g_return_if_fail(slplink != NULL);
//	g_return_if_fail(obj     != NULL);
//
//	nateonobj_data = nateon_object_to_string(obj);
//	nateonobj_base64 = purple_base64_encode((const guchar *)nateonobj_data, strlen(nateonobj_data));
//	g_free(nateonobj_data);
//
//	slpcall = nateon_slp_call_new(slplink);
//	nateon_slp_call_init(slpcall, NATEON_SLPCALL_ANY);
//
//	slpcall->data_info = g_strdup(info);
//	slpcall->cb = cb;
//	slpcall->end_cb = end_cb;
//
//	nateon_slp_call_invite(slpcall, "A4268EEC-FEC5-49E5-95C3-F126696BDBF6", 1,
//						nateonobj_base64);
//
//	g_free(nateonobj_base64);
//}
