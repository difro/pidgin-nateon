/**
 * @file msg.c Message functions
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
#include "msg.h"

//NateonMessage *
//nateon_message_new(NateonMsgType type)
//{
//	NateonMessage *msg;
//
//	msg = g_new0(NateonMessage, 1);
//	msg->type = type;
//
//#ifdef NATEON_DEBUG_MSG
//	purple_debug_info("nateon", "message new (%p)(%d)\n", msg, type);
//#endif
//
//	msg->attr_table = g_hash_table_new_full(g_str_hash, g_str_equal,
//											g_free, g_free);
//
//	nateon_message_ref(msg);
//
//	return msg;
//}
//
//void
//nateon_message_destroy(NateonMessage *msg)
//{
//	g_return_if_fail(msg != NULL);
//
//	if (msg->ref_count > 0)
//	{
//		nateon_message_unref(msg);
//
//		return;
//	}
//
//#ifdef NATEON_DEBUG_MSG
//	purple_debug_info("nateon", "message destroy (%p)\n", msg);
//#endif
//
//	if (msg->remote_user != NULL)
//		g_free(msg->remote_user);
//
//	if (msg->body != NULL)
//		g_free(msg->body);
//
//	if (msg->content_type != NULL)
//		g_free(msg->content_type);
//
//	if (msg->charset != NULL)
//		g_free(msg->charset);
//
//	g_hash_table_destroy(msg->attr_table);
//	g_list_free(msg->attr_list);
//
//	g_free(msg);
//}
//
//NateonMessage *
//nateon_message_ref(NateonMessage *msg)
//{
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	msg->ref_count++;
//
//#ifdef NATEON_DEBUG_MSG
//	purple_debug_info("nateon", "message ref (%p)[%d]\n", msg, msg->ref_count);
//#endif
//
//	return msg;
//}
//
//NateonMessage *
//nateon_message_unref(NateonMessage *msg)
//{
//	g_return_val_if_fail(msg != NULL, NULL);
//	g_return_val_if_fail(msg->ref_count > 0, NULL);
//
//	msg->ref_count--;
//
//#ifdef NATEON_DEBUG_MSG
//	purple_debug_info("nateon", "message unref (%p)[%d]\n", msg, msg->ref_count);
//#endif
//
//	if (msg->ref_count == 0)
//	{
//		nateon_message_destroy(msg);
//
//		return NULL;
//	}
//
//	return msg;
//}
//
//NateonMessage *
//nateon_message_new_plain(const char *message)
//{
//	NateonMessage *msg;
//	char *message_cr;
//
//	msg = nateon_message_new(NATEON_MSG_TEXT);
//	nateon_message_set_attr(msg, "User-Agent", "Purple/" VERSION);
//	nateon_message_set_content_type(msg, "text/plain");
//	nateon_message_set_charset(msg, "UTF-8");
//	nateon_message_set_flag(msg, 'A');
//	nateon_message_set_attr(msg, "X-MMS-IM-Format",
//						 "FN=MS%20Sans%20Serif; EF=; CO=0; PF=0");
//
//	message_cr = purple_str_add_cr(message);
//	nateon_message_set_bin_data(msg, message_cr, strlen(message_cr));
//	g_free(message_cr);
//
//	return msg;
//}
//
//NateonMessage *
//nateon_message_new_nateonslp(void)
//{
//	NateonMessage *msg;
//
//	msg = nateon_message_new(NATEON_MSG_SLP);
//
//	nateon_message_set_attr(msg, "User-Agent", NULL);
//
//	msg->nateonslp_message = TRUE;
//
//	nateon_message_set_flag(msg, 'D');
//	nateon_message_set_content_type(msg, "application/x-nateonmsgrp2p");
//
//	return msg;
//}
//
//NateonMessage *
//nateon_message_new_nudge(void)
//{
//	NateonMessage *msg;
//
//	msg = nateon_message_new(NATEON_MSG_NUDGE);
//	nateon_message_set_content_type(msg, "text/x-nateonmsgr-datacast\r\n");
//	nateon_message_set_flag(msg, 'N');
//	nateon_message_set_attr(msg,"ID","1\r\n");
//
//	return msg;
//}
//
//void
//nateon_message_parse_slp_body(NateonMessage *msg, const char *body, size_t len)
//{
//	NateonSlpHeader header;
//	const char *tmp;
//	int body_len;
//
//	tmp = body;
//
//	if (len < sizeof(header)) {
//		g_return_if_reached();
//	}
//
//	/* Import the header. */
//	memcpy(&header, tmp, sizeof(header));
//	tmp += sizeof(header);
//
//	msg->nateonslp_header.session_id = GUINT32_FROM_LE(header.session_id);
//	msg->nateonslp_header.id         = GUINT32_FROM_LE(header.id);
//	msg->nateonslp_header.offset     = GUINT64_FROM_LE(header.offset);
//	msg->nateonslp_header.total_size = GUINT64_FROM_LE(header.total_size);
//	msg->nateonslp_header.length     = GUINT32_FROM_LE(header.length);
//	msg->nateonslp_header.flags      = GUINT32_FROM_LE(header.flags);
//	msg->nateonslp_header.ack_id     = GUINT32_FROM_LE(header.ack_id);
//	msg->nateonslp_header.ack_sub_id = GUINT32_FROM_LE(header.ack_sub_id);
//	msg->nateonslp_header.ack_size   = GUINT64_FROM_LE(header.ack_size);
//
//	/* Import the body. */
//	body_len = len - (tmp - body);
//	/* msg->body_len = msg->nateonslp_header.length; */
//
//	if (body_len > 0) {
//		msg->body_len = len - (tmp - body);
//		msg->body = g_malloc0(msg->body_len + 1);
//		memcpy(msg->body, tmp, msg->body_len);
//		tmp += body_len;
//	}
//}
//
//void
//nateon_message_parse_payload(NateonMessage *msg,
//						  const char *payload, size_t payload_len)
//{
//	char *tmp_base, *tmp;
//	const char *content_type;
//	char *end;
//	char **elems, **cur, **tokens;
//
//	g_return_if_fail(payload != NULL);
//
//	tmp_base = tmp = g_malloc0(payload_len + 1);
//	memcpy(tmp_base, payload, payload_len);
//
//	/* Parse the attributes. */
//	end = strstr(tmp, "\r\n\r\n");
//	/* TODO? some clients use \r delimiters instead of \r\n, the official client
//	 * doesn't send such messages, but does handle receiving them. We'll just
//	 * avoid crashing for now */
//	if (end == NULL) {
//		g_free(tmp_base);
//		g_return_if_reached();
//	}
//	*end = '\0';
//
//	elems = g_strsplit(tmp, "\r\n", 0);
//
//	for (cur = elems; *cur != NULL; cur++)
//	{
//		const char *key, *value;
//
//		tokens = g_strsplit(*cur, ": ", 2);
//
//		key = tokens[0];
//		value = tokens[1];
//
//		if (!strcmp(key, "MIME-Version"))
//		{
//			g_strfreev(tokens);
//			continue;
//		}
//
//		if (!strcmp(key, "Content-Type"))
//		{
//			char *charset, *c;
//
//			if ((c = strchr(value, ';')) != NULL)
//			{
//				if ((charset = strchr(c, '=')) != NULL)
//				{
//					charset++;
//					nateon_message_set_charset(msg, charset);
//				}
//
//				*c = '\0';
//			}
//
//			nateon_message_set_content_type(msg, value);
//		}
//		else
//		{
//			nateon_message_set_attr(msg, key, value);
//		}
//
//		g_strfreev(tokens);
//	}
//
//	g_strfreev(elems);
//
//	/* Proceed to the end of the "\r\n\r\n" */
//	tmp = end + 4;
//
//	/* Now we *should* be at the body. */
//	content_type = nateon_message_get_content_type(msg);
//
//	if (content_type != NULL &&
//		!strcmp(content_type, "application/x-nateonmsgrp2p"))
//	{
//		NateonSlpHeader header;
//		NateonSlpFooter footer;
//		int body_len;
//
//		if (payload_len - (tmp - tmp_base) < sizeof(header)) {
//			g_free(tmp_base);
//			g_return_if_reached();
//		}
//
//		msg->nateonslp_message = TRUE;
//
//		/* Import the header. */
//		memcpy(&header, tmp, sizeof(header));
//		tmp += sizeof(header);
//
//		msg->nateonslp_header.session_id = GUINT32_FROM_LE(header.session_id);
//		msg->nateonslp_header.id         = GUINT32_FROM_LE(header.id);
//		msg->nateonslp_header.offset     = GUINT64_FROM_LE(header.offset);
//		msg->nateonslp_header.total_size = GUINT64_FROM_LE(header.total_size);
//		msg->nateonslp_header.length     = GUINT32_FROM_LE(header.length);
//		msg->nateonslp_header.flags      = GUINT32_FROM_LE(header.flags);
//		msg->nateonslp_header.ack_id     = GUINT32_FROM_LE(header.ack_id);
//		msg->nateonslp_header.ack_sub_id = GUINT32_FROM_LE(header.ack_sub_id);
//		msg->nateonslp_header.ack_size   = GUINT64_FROM_LE(header.ack_size);
//
//		body_len = payload_len - (tmp - tmp_base) - sizeof(footer);
//
//		/* Import the body. */
//		if (body_len > 0) {
//			msg->body_len = body_len;
//			msg->body = g_malloc0(msg->body_len + 1);
//			memcpy(msg->body, tmp, msg->body_len);
//			tmp += body_len;
//		}
//
//		/* Import the footer. */
//		if (body_len >= 0) {
//			memcpy(&footer, tmp, sizeof(footer));
//			tmp += sizeof(footer);
//			msg->nateonslp_footer.value = GUINT32_FROM_BE(footer.value);
//		}
//	}
//	else
//	{
//		if (payload_len - (tmp - tmp_base) > 0) {
//			msg->body_len = payload_len - (tmp - tmp_base);
//			msg->body = g_malloc0(msg->body_len + 1);
//			memcpy(msg->body, tmp, msg->body_len);
//		}
//	}
//
//	g_free(tmp_base);
//}
//
//NateonMessage *
//nateon_message_new_from_cmd(NateonSession *session, NateonCommand *cmd)
//{
//	NateonMessage *msg;
//
//	g_return_val_if_fail(cmd != NULL, NULL);
//
//	msg = nateon_message_new(NATEON_MSG_UNKNOWN);
//
//	msg->remote_user = g_strdup(cmd->params[0]);
//	/* msg->size = atoi(cmd->params[2]); */
//	msg->cmd = cmd;
//
//	return msg;
//}
//
//char *
//nateon_message_gen_slp_body(NateonMessage *msg, size_t *ret_size)
//{
//	NateonSlpHeader header;
//
//	char *tmp, *base;
//	const void *body;
//	size_t len, body_len;
//
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	len = NATEON_BUF_LEN;
//
//	base = tmp = g_malloc(len + 1);
//
//	body = nateon_message_get_bin_data(msg, &body_len);
//
//	header.session_id = GUINT32_TO_LE(msg->nateonslp_header.session_id);
//	header.id         = GUINT32_TO_LE(msg->nateonslp_header.id);
//	header.offset     = GUINT64_TO_LE(msg->nateonslp_header.offset);
//	header.total_size = GUINT64_TO_LE(msg->nateonslp_header.total_size);
//	header.length     = GUINT32_TO_LE(msg->nateonslp_header.length);
//	header.flags      = GUINT32_TO_LE(msg->nateonslp_header.flags);
//	header.ack_id     = GUINT32_TO_LE(msg->nateonslp_header.ack_id);
//	header.ack_sub_id = GUINT32_TO_LE(msg->nateonslp_header.ack_sub_id);
//	header.ack_size   = GUINT64_TO_LE(msg->nateonslp_header.ack_size);
//
//	memcpy(tmp, &header, 48);
//	tmp += 48;
//
//	if (body != NULL)
//	{
//		memcpy(tmp, body, body_len);
//		tmp += body_len;
//	}
//
//	if (ret_size != NULL)
//		*ret_size = tmp - base;
//
//	return base;
//}
//
//char *
//nateon_message_gen_payload(NateonMessage *msg, size_t *ret_size)
//{
//	GList *l;
//	char *n, *base, *end;
//	int len;
//	size_t body_len;
//	const void *body;
//
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	len = NATEON_BUF_LEN;
//
//	base = n = end = g_malloc(len + 1);
//	end += len;
//
//	/* Standard header. */
//	if (msg->charset == NULL)
//	{
//		g_snprintf(n, len,
//				   "MIME-Version: 1.0\r\n"
//				   "Content-Type: %s\r\n",
//				   msg->content_type);
//	}
//	else
//	{
//		g_snprintf(n, len,
//				   "MIME-Version: 1.0\r\n"
//				   "Content-Type: %s; charset=%s\r\n",
//				   msg->content_type, msg->charset);
//	}
//
//	n += strlen(n);
//
//	for (l = msg->attr_list; l != NULL; l = l->next)
//	{
//		const char *key;
//		const char *value;
//
//		key = l->data;
//		value = nateon_message_get_attr(msg, key);
//
//		g_snprintf(n, end - n, "%s: %s\r\n", key, value);
//		n += strlen(n);
//	}
//
//	n += g_strlcpy(n, "\r\n", end - n);
//
//	body = nateon_message_get_bin_data(msg, &body_len);
//
//	if (msg->nateonslp_message)
//	{
//		NateonSlpHeader header;
//		NateonSlpFooter footer;
//
//		header.session_id = GUINT32_TO_LE(msg->nateonslp_header.session_id);
//		header.id         = GUINT32_TO_LE(msg->nateonslp_header.id);
//		header.offset     = GUINT64_TO_LE(msg->nateonslp_header.offset);
//		header.total_size = GUINT64_TO_LE(msg->nateonslp_header.total_size);
//		header.length     = GUINT32_TO_LE(msg->nateonslp_header.length);
//		header.flags      = GUINT32_TO_LE(msg->nateonslp_header.flags);
//		header.ack_id     = GUINT32_TO_LE(msg->nateonslp_header.ack_id);
//		header.ack_sub_id = GUINT32_TO_LE(msg->nateonslp_header.ack_sub_id);
//		header.ack_size   = GUINT64_TO_LE(msg->nateonslp_header.ack_size);
//
//		memcpy(n, &header, 48);
//		n += 48;
//
//		if (body != NULL)
//		{
//			memcpy(n, body, body_len);
//
//			n += body_len;
//		}
//
//		footer.value = GUINT32_TO_BE(msg->nateonslp_footer.value);
//
//		memcpy(n, &footer, 4);
//		n += 4;
//	}
//	else
//	{
//		if (body != NULL)
//		{
//			memcpy(n, body, body_len);
//			n += body_len;
//		}
//	}
//
//	if (ret_size != NULL)
//	{
//		*ret_size = n - base;
//
//		if (*ret_size > 1664)
//			*ret_size = 1664;
//	}
//
//	return base;
//}
//
//void
//nateon_message_set_flag(NateonMessage *msg, char flag)
//{
//	g_return_if_fail(msg != NULL);
//	g_return_if_fail(flag != 0);
//
//	msg->flag = flag;
//}
//
//char
//nateon_message_get_flag(const NateonMessage *msg)
//{
//	g_return_val_if_fail(msg != NULL, 0);
//
//	return msg->flag;
//}
//
//void
//nateon_message_set_bin_data(NateonMessage *msg, const void *data, size_t len)
//{
//	g_return_if_fail(msg != NULL);
//
//	/* There is no need to waste memory on data we cannot send anyway */
//	if (len > 1664)
//		len = 1664;
//
//	if (msg->body != NULL)
//		g_free(msg->body);
//
//	if (data != NULL && len > 0)
//	{
//		msg->body = g_malloc0(len + 1);
//		memcpy(msg->body, data, len);
//		msg->body_len = len;
//	}
//	else
//	{
//		msg->body = NULL;
//		msg->body_len = 0;
//	}
//}
//
//const void *
//nateon_message_get_bin_data(const NateonMessage *msg, size_t *len)
//{
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	if (len)
//		*len = msg->body_len;
//
//	return msg->body;
//}
//
//void
//nateon_message_set_content_type(NateonMessage *msg, const char *type)
//{
//	g_return_if_fail(msg != NULL);
//
//	if (msg->content_type != NULL)
//		g_free(msg->content_type);
//
//	msg->content_type = (type != NULL) ? g_strdup(type) : NULL;
//}
//
//const char *
//nateon_message_get_content_type(const NateonMessage *msg)
//{
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	return msg->content_type;
//}
//
//void
//nateon_message_set_charset(NateonMessage *msg, const char *charset)
//{
//	g_return_if_fail(msg != NULL);
//
//	if (msg->charset != NULL)
//		g_free(msg->charset);
//
//	msg->charset = (charset != NULL) ? g_strdup(charset) : NULL;
//}
//
//const char *
//nateon_message_get_charset(const NateonMessage *msg)
//{
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	return msg->charset;
//}
//
//void
//nateon_message_set_attr(NateonMessage *msg, const char *attr, const char *value)
//{
//	const char *temp;
//	char *new_attr;
//
//	g_return_if_fail(msg != NULL);
//	g_return_if_fail(attr != NULL);
//
//	temp = nateon_message_get_attr(msg, attr);
//
//	if (value == NULL)
//	{
//		if (temp != NULL)
//		{
//			GList *l;
//
//			for (l = msg->attr_list; l != NULL; l = l->next)
//			{
//				if (!g_ascii_strcasecmp(l->data, attr))
//				{
//					msg->attr_list = g_list_remove(msg->attr_list, l->data);
//
//					break;
//				}
//			}
//
//			g_hash_table_remove(msg->attr_table, attr);
//		}
//
//		return;
//	}
//
//	new_attr = g_strdup(attr);
//
//	g_hash_table_insert(msg->attr_table, new_attr, g_strdup(value));
//
//	if (temp == NULL)
//		msg->attr_list = g_list_append(msg->attr_list, new_attr);
//}
//
//const char *
//nateon_message_get_attr(const NateonMessage *msg, const char *attr)
//{
//	g_return_val_if_fail(msg != NULL, NULL);
//	g_return_val_if_fail(attr != NULL, NULL);
//
//	return g_hash_table_lookup(msg->attr_table, attr);
//}
//
//GHashTable *
//nateon_message_get_hashtable_from_body(const NateonMessage *msg)
//{
//	GHashTable *table;
//	size_t body_len;
//	const char *body;
//	char **elems, **cur, **tokens, *body_str;
//
//	g_return_val_if_fail(msg != NULL, NULL);
//
//	table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
//
//	body = nateon_message_get_bin_data(msg, &body_len);
//
//	g_return_val_if_fail(body != NULL, NULL);
//
//	body_str = g_strndup(body, body_len);
//	elems = g_strsplit(body_str, "\r\n", 0);
//	g_free(body_str);
//
//	for (cur = elems; *cur != NULL; cur++)
//	{
//		if (**cur == '\0')
//			break;
//
//		tokens = g_strsplit(*cur, ": ", 2);
//
//		if (tokens[0] != NULL && tokens[1] != NULL)
//			g_hash_table_insert(table, tokens[0], tokens[1]);
//
//		g_free(tokens);
//	}
//
//	g_strfreev(elems);
//
//	return table;
//}
//
//char *
//nateon_message_to_string(NateonMessage *msg)
//{
//	size_t body_len;
//	const char *body;
//
//	g_return_val_if_fail(msg != NULL, NULL);
//	g_return_val_if_fail(msg->type == NATEON_MSG_TEXT, NULL);
//
//	body = nateon_message_get_bin_data(msg, &body_len);
//
//	return g_strndup(body, body_len);
//}
//
//void
//nateon_message_show_readable(NateonMessage *msg, const char *info,
//						  gboolean text_body)
//{
//	GString *str;
//	size_t body_len;
//	const char *body;
//	GList *l;
//
//	g_return_if_fail(msg != NULL);
//
//	str = g_string_new(NULL);
//
//	/* Standard header. */
//	if (msg->charset == NULL)
//	{
//		g_string_append_printf(str,
//				   "MIME-Version: 1.0\r\n"
//				   "Content-Type: %s\r\n",
//				   msg->content_type);
//	}
//	else
//	{
//		g_string_append_printf(str,
//				   "MIME-Version: 1.0\r\n"
//				   "Content-Type: %s; charset=%s\r\n",
//				   msg->content_type, msg->charset);
//	}
//
//	for (l = msg->attr_list; l; l = l->next)
//	{
//		char *key;
//		const char *value;
//
//		key = l->data;
//		value = nateon_message_get_attr(msg, key);
//
//		g_string_append_printf(str, "%s: %s\r\n", key, value);
//	}
//
//	g_string_append(str, "\r\n");
//
//	body = nateon_message_get_bin_data(msg, &body_len);
//
//	if (msg->nateonslp_message)
//	{
//		g_string_append_printf(str, "Session ID: %u\r\n", msg->nateonslp_header.session_id);
//		g_string_append_printf(str, "ID:         %u\r\n", msg->nateonslp_header.id);
//		g_string_append_printf(str, "Offset:     %" G_GUINT64_FORMAT "\r\n", msg->nateonslp_header.offset);
//		g_string_append_printf(str, "Total size: %" G_GUINT64_FORMAT "\r\n", msg->nateonslp_header.total_size);
//		g_string_append_printf(str, "Length:     %u\r\n", msg->nateonslp_header.length);
//		g_string_append_printf(str, "Flags:      0x%x\r\n", msg->nateonslp_header.flags);
//		g_string_append_printf(str, "ACK ID:     %u\r\n", msg->nateonslp_header.ack_id);
//		g_string_append_printf(str, "SUB ID:     %u\r\n", msg->nateonslp_header.ack_sub_id);
//		g_string_append_printf(str, "ACK Size:   %" G_GUINT64_FORMAT "\r\n", msg->nateonslp_header.ack_size);
//
//#ifdef NATEON_DEBUG_SLP_VERBOSE
//		if (body != NULL)
//		{
//			if (text_body)
//			{
//				g_string_append_len(str, body, body_len);
//				if (body[body_len - 1] == '\0')
//				{
//					str->len--;
//					g_string_append(str, " 0x00");
//				}
//				g_string_append(str, "\r\n");
//			}
//			else
//			{
//				int i;
//				for (i = 0; i < msg->body_len; i++)
//				{
//					g_string_append_printf(str, "%.2hhX ", body[i]);
//					if ((i % 16) == 15)
//						g_string_append(str, "\r\n");
//				}
//				g_string_append(str, "\r\n");
//			}
//		}
//#endif
//
//		g_string_append_printf(str, "Footer:     %u\r\n", msg->nateonslp_footer.value);
//	}
//	else
//	{
//		if (body != NULL)
//		{
//			g_string_append_len(str, body, body_len);
//			g_string_append(str, "\r\n");
//		}
//	}
//
//	purple_debug_info("nateon", "Message %s:\n{%s}\n", info, str->str);
//
//	g_string_free(str, TRUE);
//}
