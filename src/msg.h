/**
 * @file msg.h Message functions
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
#ifndef _NATEON_MSG_H_
#define _NATEON_MSG_H_

typedef struct _NateonMessage NateonMessage;

#include "session.h"
//#include "user.h"

#include "command.h"
#include "transaction.h"

//typedef void (*NateonMsgCb)(NateonMessage *, void *data);

///*
//typedef enum
//{
//	NATEON_MSG_NORMAL,
//	NATEON_MSG_SLP_SB,
//	NATEON_MSG_SLP_DC
//
//} NateonMsgType;
//*/

typedef enum
{
	NATEON_MSG_UNKNOWN,
	NATEON_MSG_TEXT,
	NATEON_MSG_TYPING,
//	NATEON_MSG_CAPS,
//	NATEON_MSG_SLP,
//	NATEON_MSG_NUDGE

} NateonMsgType;

typedef enum
{
	NATEON_MSG_ERROR_NONE, /**< No error. */
	NATEON_MSG_ERROR_TIMEOUT, /**< The message timedout. */
	NATEON_MSG_ERROR_NAK, /**< The message could not be sent. */
	NATEON_MSG_ERROR_SB, /**< The error comes from the switchboard. */
	NATEON_MSG_ERROR_UNKNOWN /**< An unknown error occurred. */
} NateonMsgErrorType;

//typedef struct
//{
//	guint32 session_id;
//	guint32 id;
//	guint64 offset;
//	guint64 total_size;
//	guint32 length;
//	guint32 flags;
//	guint32 ack_id;
//	guint32 ack_sub_id;
//	guint64 ack_size;
//
//} NateonSlpHeader;
//
//typedef struct
//{
//	guint32 value;
//
//} NateonSlpFooter;

/**
 * A message.
 */
struct _NateonMessage
{
	size_t ref_count;           /**< The reference count.       */

	NateonMsgType type;
//
//	gboolean nateonslp_message;
//
//	char *remote_user;
//	char flag;
//
//	char *content_type;
//	char *charset;
	char *body;
	gsize body_len;

//	NateonSlpHeader nateonslp_header;
//	NateonSlpFooter nateonslp_footer;
//
//	GHashTable *attr_table;
//	GList *attr_list;

	gboolean ack_ref;           /**< A flag that states if this message has
								  been ref'ed for using it in a callback. */

//	NateonCommand *cmd;
	NateonTransaction *trans;

//	NateonMsgCb ack_cb; /**< The callback to call when we receive an ACK of this
//					   message. */
//	NateonMsgCb nak_cb; /**< The callback to call when we receive a NAK of this
//					   message. */
//	void *ack_data; /**< The data used by callbacks. */
//
//	NateonMsgErrorType error; /**< The error of the message. */
};

/**
 * Creates a new, empty message.
 *
 * @return A new message.
 */
NateonMessage *nateon_message_new(NateonMsgType type);

///**
// * Creates a new, empty NATEONSLP message.
// *
// * @return A new NATEONSLP message.
// */
//NateonMessage *nateon_message_new_nateonslp(void);
//
///**
// * Creates a new nudge message.
// *
// * @return A new nudge message.
// */
//NateonMessage *nateon_message_new_nudge(void);

/**
 * Creates a new plain message.
 *
 * @return A new plain message.
 */
NateonMessage *nateon_message_new_plain(const char *message);

///**
// * Creates a NATEONSLP ack message.
// *
// * @param acked_msg The message to acknowledge.
// *
// * @return A new NATEONSLP ack message.
// */
//NateonMessage *nateon_message_new_nateonslp_ack(NateonMessage *acked_msg);
//
///**
// * Creates a new message based off a command.
// *
// * @param session The NATEON session.
// * @param cmd     The command.
// *
// * @return The new message.
// */
//NateonMessage *nateon_message_new_from_cmd(NateonSession *session, NateonCommand *cmd);
//
///**
// * Parses the payload of a message.
// *
// * @param msg         The message.
// * @param payload     The payload.
// * @param payload_len The length of the payload.
// */
//void nateon_message_parse_payload(NateonMessage *msg, const char *payload,
//							   size_t payload_len);

/**
 * Destroys a message.
 *
 * @param msg The message to destroy.
 */
void nateon_message_destroy(NateonMessage *msg);

/**
 * Increments the reference count on a message.
 *
 * @param msg The message.
 *
 * @return @a msg
 */
NateonMessage *nateon_message_ref(NateonMessage *msg);

/**
 * Decrements the reference count on a message.
 *
 * This will destroy the structure if the count hits 0.
 *
 * @param msg The message.
 *
 * @return @a msg, or @c NULL if the new count is 0.
 */
NateonMessage *nateon_message_unref(NateonMessage *msg);

///**
// * Generates the payload data of a message.
// *
// * @param msg      The message.
// * @param ret_size The returned size of the payload.
// *
// * @return The payload data of the message.
// */
//char *nateon_message_gen_payload(NateonMessage *msg, size_t *ret_size);
//
///**
// * Sets the flag for an outgoing message.
// *
// * @param msg  The message.
// * @param flag The flag.
// */
//void nateon_message_set_flag(NateonMessage *msg, char flag);
//
///**
// * Returns the flag for an outgoing message.
// *
// * @param msg The message.
// *
// * @return The flag.
// */
//char nateon_message_get_flag(const NateonMessage *msg);
//
//#if 0
///**
// * Sets the body of a message.
// *
// * @param msg  The message.
// * @param body The body of the message.
// */
//void nateon_message_set_body(NateonMessage *msg, const char *body);
//
///**
// * Returns the body of the message.
// *
// * @param msg The message.
// *
// * @return The body of the message.
// */
//const char *nateon_message_get_body(const NateonMessage *msg);
//#endif
/**
 * Sets the binary content of the message.
 *
 * @param msg  The message.
 * @param data The binary data.
 * @param len  The length of the data.
 */
void nateon_message_set_bin_data(NateonMessage *msg, const void *data, size_t len);

///**
// * Returns the binary content of the message.
// *
// * @param msg The message.
// * @param len The returned length of the data.
// *
// * @return The binary data.
// */
//const void *nateon_message_get_bin_data(const NateonMessage *msg, size_t *len);
//
///**
// * Sets the content type in a message.
// *
// * @param msg  The message.
// * @param type The content-type.
// */
//void nateon_message_set_content_type(NateonMessage *msg, const char *type);
//
///**
// * Returns the content type in a message.
// *
// * @param msg The message.
// *
// * @return The content-type.
// */
//const char *nateon_message_get_content_type(const NateonMessage *msg);
//
///**
// * Sets the charset in a message.
// *
// * @param msg     The message.
// * @param charset The charset.
// */
//void nateon_message_set_charset(NateonMessage *msg, const char *charset);
//
///**
// * Returns the charset in a message.
// *
// * @param msg The message.
// *
// * @return The charset.
// */
//const char *nateon_message_get_charset(const NateonMessage *msg);
//
///**
// * Sets an attribute in a message.
// *
// * @param msg   The message.
// * @param attr  The attribute name.
// * @param value The attribute value.
// */
//void nateon_message_set_attr(NateonMessage *msg, const char *attr,
//						  const char *value);
//
///**
// * Returns an attribute from a message.
// *
// * @param msg  The message.
// * @param attr The attribute.
// *
// * @return The value, or @c NULL if not found.
// */
//const char *nateon_message_get_attr(const NateonMessage *msg, const char *attr);
//
///**
// * Parses the body and returns it in the form of a hashtable.
// *
// * @param msg The message.
// *
// * @return The resulting hashtable.
// */
//GHashTable *nateon_message_get_hashtable_from_body(const NateonMessage *msg);
//
//void nateon_message_show_readable(NateonMessage *msg, const char *info,
//							   gboolean text_body);
//
//void nateon_message_parse_slp_body(NateonMessage *msg, const char *body,
//								size_t len);
//
//char *nateon_message_gen_slp_body(NateonMessage *msg, size_t *ret_size);
//
//char *nateon_message_to_string(NateonMessage *msg);

#endif /* _NATEON_MSG_H_ */
