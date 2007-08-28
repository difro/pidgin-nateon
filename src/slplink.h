/**
 * @file slplink.h NATEONSLP Link support
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
#ifndef _NATEON_SLPLINK_H_
#define _NATEON_SLPLINK_H_

typedef struct _NateonSlpLink NateonSlpLink;

//#include "directconn.h"
//#include "slpcall.h"
//#include "slpmsg.h"

#include "switchboard.h"

//#include "ft.h"

#include "session.h"

//typedef void (*NateonSlpCb)(NateonSlpCall *slpcall,
//						 const guchar *data, gsize size);
//typedef void (*NateonSlpEndCb)(NateonSlpCall *slpcall, NateonSession *session);

struct _NateonSlpLink
{
	NateonSession *session;
	NateonSwitchBoard *swboard;

	char *local_user;
	char *remote_user;

//	int slp_seq_id;
//
//	NateonDirectConn *directconn;
//
//	GList *slp_calls;
//	GList *slp_sessions;
//	GList *slp_msgs;
//
//	GQueue *slp_msg_queue;
};

NateonSlpLink *nateon_slplink_new(NateonSession *session, const char *username);
//void nateon_slplink_destroy(NateonSlpLink *slplink);
NateonSlpLink *nateon_session_find_slplink(NateonSession *session, const char *who);
NateonSlpLink *nateon_session_get_slplink(NateonSession *session, const char *username);
//NateonSlpSession *nateon_slplink_find_slp_session(NateonSlpLink *slplink,
//											long session_id);
//void nateon_slplink_add_slpcall(NateonSlpLink *slplink, NateonSlpCall *slpcall);
//void nateon_slplink_remove_slpcall(NateonSlpLink *slplink, NateonSlpCall *slpcall);
//NateonSlpCall *nateon_slplink_find_slp_call(NateonSlpLink *slplink,
//									  const char *id);
//NateonSlpCall *nateon_slplink_find_slp_call_with_session_id(NateonSlpLink *slplink, long id);
//void nateon_slplink_send_msg(NateonSlpLink *slplink, NateonMessage *msg);
//void nateon_slplink_release_slpmsg(NateonSlpLink *slplink,
//								NateonSlpMessage *slpmsg);
//void nateon_slplink_queue_slpmsg(NateonSlpLink *slplink, NateonSlpMessage *slpmsg);
//void nateon_slplink_send_slpmsg(NateonSlpLink *slplink,
//							 NateonSlpMessage *slpmsg);
//void nateon_slplink_unleash(NateonSlpLink *slplink);
//void nateon_slplink_send_ack(NateonSlpLink *slplink, NateonMessage *msg);
//void nateon_slplink_process_msg(NateonSlpLink *slplink, NateonMessage *msg);
//NateonSlpMessage *nateon_slplink_message_find(NateonSlpLink *slplink, long session_id, long id);
//void nateon_slplink_append_slp_msg(NateonSlpLink *slplink, NateonSlpMessage *slpmsg);
//void nateon_slplink_remove_slp_msg(NateonSlpLink *slplink,
//								NateonSlpMessage *slpmsg);
//void nateon_slplink_request_ft(NateonSlpLink *slplink, PurpleXfer *xfer);
//
//void nateon_slplink_request_object(NateonSlpLink *slplink,
//								const char *info,
//								NateonSlpCb cb,
//								NateonSlpEndCb end_cb,
//								const NateonObject *obj);
//
//NateonSlpCall *nateon_slp_process_msg(NateonSlpLink *slplink, NateonSlpMessage *slpmsg);

#endif /* _NATEON_SLPLINK_H_ */
