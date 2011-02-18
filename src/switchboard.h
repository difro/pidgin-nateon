/**
 * @file switchboard.h NATEON switchboard functions
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
#ifndef _NATEON_SWITCHBOARD_H_
#define _NATEON_SWITCHBOARD_H_

typedef struct _NateonSwitchBoard NateonSwitchBoard;

#include "conversation.h"

#include "msg.h"
#include "user.h"

#include "servconn.h"

//#include "slplink.h"

/**
 * A switchboard error.
 */
typedef enum
{
	NATEON_SB_ERROR_NONE, /**< No error. */
	NATEON_SB_ERROR_INVT, /**< The user could not join */
//	NATEON_SB_ERROR_OFFLINE, /**< The account is offline. */
//	NATEON_SB_ERROR_USER_OFFLINE, /**< The user to call is offline. */
	NATEON_SB_ERROR_CONNECTION, /**< There was a connection error. */
//	NATEON_SB_ERROR_TOO_FAST, /**< We are sending too fast */
	NATEON_SB_ERROR_UNKNOWN /**< An unknown error occurred. */
} NateonSBErrorType;

/**
 * A switchboard flag.
 */
typedef enum
{
	NATEON_SB_FLAG_IM = 0x01, /**< This switchboard is being used for a conversation. */
	NATEON_SB_FLAG_FT = 0x02, /**< This switchboard is being used for file transfer. */

} NateonSBFlag;

/**
 * A switchboard.
 *
 * A place where a bunch of users send messages to the rest of the users.
 */
struct _NateonSwitchBoard
{
	NateonSession *session;
	NateonServConn *servconn;
	NateonCmdProc *cmdproc;
	char *im_user; /* Seems to be my NateOn ID */

	NateonSBFlag flag;
	char *auth_key;
//	char *session_id;

	PurpleConversation *conv; /**< The conversation that displays the
							  messages of this switchboard, or @c NULL if
							  this is a helper switchboard. */

	gboolean empty;			/**< A flag that states if the swithcboard has no
							  users in it. */
	gboolean invited;		/**< A flag that states if we were invited to the
							  switchboard. */
	gboolean ready;			/**< A flag that states if this switchboard is
							  ready to be used. */
	gboolean closed;		/**< A flag that states if the switchboard has
							  been closed by the user. */
	gboolean destroying;	/**< A flag that states if the switchboard is
							  alredy on the process of destruction. */

	int current_users;
	int total_users;
	GList *users;

	int chat_id;

	GQueue *msg_queue; /**< Queue of messages to send. */
	GList *ack_list; /**< List of messages waiting for an ack. */

	NateonSBErrorType error; /**< The error that occurred in this switchboard (if applicable). */
//	NateonSlpLink *slplink; /**< The slplink that is using this switchboard. */
};

/**
 * Initialize the variables for switchboard creation.
 */
void nateon_switchboard_init(void);

/**
 * Destroy the variables for switchboard creation.
 */
void nateon_switchboard_end(void);

/**
 * Creates a new switchboard.
 *
 * @param session The NATEON session.
 *
 * @return The new switchboard.
 */
NateonSwitchBoard *nateon_switchboard_new(NateonSession *session);

/**
 * Destroys a switchboard.
 *
 * @param swboard The switchboard to destroy.
 */
void nateon_switchboard_destroy(NateonSwitchBoard *swboard);

/**
 * Sets the auth key the switchboard must use when connecting.
 *
 * @param swboard The switchboard.
 * @param key     The auth key.
 */
void nateon_switchboard_set_auth_key(NateonSwitchBoard *swboard, const char *key);

///**
// * Returns the auth key the switchboard must use when connecting.
// *
// * @param swboard The switchboard.
// *
// * @return The auth key.
// */
//const char *nateon_switchboard_get_auth_key(NateonSwitchBoard *swboard);
//
///**
// * Sets the session ID the switchboard must use when connecting.
// *
// * @param swboard The switchboard.
// * @param id      The session ID.
// */
//void nateon_switchboard_set_session_id(NateonSwitchBoard *swboard, const char *id);
//
///**
// * Returns the session ID the switchboard must use when connecting.
// *
// * @param swboard The switchboard.
// *
// * @return The session ID.
// */
//const char *nateon_switchboard_get_session_id(NateonSwitchBoard *swboard);

/**
 * Sets whether or not we were invited to this switchboard.
 *
 * @param swboard The switchboard.
 * @param invite  @c TRUE if invited, @c FALSE otherwise.
 */
void nateon_switchboard_set_invited(NateonSwitchBoard *swboard, gboolean invited);

/**
 * Returns whether or not we were invited to this switchboard.
 *
 * @param swboard The switchboard.
 *
 * @return @c TRUE if invited, @c FALSE otherwise.
 */
gboolean nateon_switchboard_is_invited(NateonSwitchBoard *swboard);

///**
// * Connects to a switchboard.
// *
// * @param swboard The switchboard.
// * @param host    The switchboard server host.
// * @param port    The switcbharod server port.
// *
// * @return @c TRUE if able to connect, or @c FALSE otherwise.
// */
gboolean nateon_switchboard_connect(NateonSwitchBoard *swboard, const char *host, int port);

/**
 * Disconnects from a switchboard.
 *
 * @param swboard The switchboard to disconnect from.
 */
void nateon_switchboard_disconnect(NateonSwitchBoard *swboard);

/**
 * Closes the switchboard.
 *
 * Called when a conversation is closed.
 *
 * @param swboard The switchboard to close.
 */
void nateon_switchboard_close(NateonSwitchBoard *swboard);

/**
 * Release a switchboard from a certain function.
 *
 * @param swboard The switchboard to release.
 * @param flag The flag that states the function.
 *
 * @return @c TRUE if the switchboard was closed, @c FALSE otherwise.
 */
gboolean nateon_switchboard_release(NateonSwitchBoard *swboard, NateonSBFlag flag);

/**
 * Returns whether or not we currently can send a message through this
 * switchboard.
 *
 * @param swboard The switchboard.
 *
 * @return @c TRUE if a message can be sent, @c FALSE otherwise.
 */
gboolean nateon_switchboard_can_send(NateonSwitchBoard *swboard);

/**
 * Sends a message through this switchboard.
 *
 * @param swboard The switchboard.
 * @param msg The message.
 * @param queue A flag that states if we want this message to be queued (in
 * the case it cannot currently be sent).
 *
 * @return @c TRUE if a message can be sent, @c FALSE otherwise.
 */
void nateon_switchboard_send_msg(NateonSwitchBoard *swboard, NateonMessage *msg, gboolean queue);

//gboolean nateon_switchboard_chat_leave(NateonSwitchBoard *swboard);
//gboolean nateon_switchboard_chat_invite(NateonSwitchBoard *swboard, const char *who);

void nateon_switchboard_request(NateonSwitchBoard *swboard);
void nateon_switchboard_request_add_user(NateonSwitchBoard *swboard, const char *user);

///**
// * Processes peer to peer messages.
// *
// * @param cmdproc The command processor.
// * @param msg     The message.
// */
//void nateon_p2p_msg(NateonCmdProc *cmdproc, NateonMessage *msg);
//
///**
// * Processes emoticon messages.
// *
// * @param cmdproc The command processor.
// * @param msg     The message.
// */
//void nateon_emoticon_msg(NateonCmdProc *cmdproc, NateonMessage *msg);
//
///**
// * Processes INVITE messages.
// *
// * @param cmdproc The command processor.
// * @param msg     The message.
// */
//void nateon_invite_msg(NateonCmdProc *cmdproc, NateonMessage *msg);

#endif /* _NATEON_SWITCHBOARD_H_ */
