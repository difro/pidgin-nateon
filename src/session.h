/**
 * @file session.h NATEON session functions
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
#ifndef _NATEON_SESSION_H_
#define _NATEON_SESSION_H_

typedef struct _NateonSession NateonSession;

//#include "sslconn.h"

#include "user.h"
//#include "slpcall.h"

#include "notification.h"
#include "switchboard.h"
#include "group.h"

#include "cmdproc.h"
//#include "nexus.h"
//#include "httpconn.h"

#include "userlist.h"
#include "sync.h"

/**
 * Types of errors.
 */
typedef enum
{
	NATEON_ERROR_SERVCONN,
	NATEON_ERROR_UNSUPPORTED_PROTOCOL,
	NATEON_ERROR_HTTP_MALFORMED,
	NATEON_ERROR_AUTH,
	NATEON_ERROR_BAD_BLIST,
	NATEON_ERROR_SIGN_OTHER,
	NATEON_ERROR_SERV_DOWN,
	NATEON_ERROR_SERV_UNAVAILABLE

} NateonErrorType;

/**
 * Login steps.
 */
typedef enum
{
	NATEON_LOGIN_STEP_START,
	NATEON_LOGIN_STEP_HANDSHAKE,
	NATEON_LOGIN_STEP_TRANSFER,
	NATEON_LOGIN_STEP_AUTH_START,
	NATEON_LOGIN_STEP_AUTH,
	NATEON_LOGIN_STEP_GET_COOKIE,
	NATEON_LOGIN_STEP_AUTH_END,
	NATEON_LOGIN_STEP_SYN,
	NATEON_LOGIN_STEP_END

} NateonLoginStep;

#define NATEON_LOGIN_STEPS NATEON_LOGIN_STEP_END

struct _NateonSession
{
	PurpleAccount *account;
	NateonUser *user;

	gfloat protocol_ver;

	NateonLoginStep login_step; /**< The current step in the login process. */

	gboolean connected;
	gboolean logged_in; /**< A temporal flag to ignore local buddy list adds. */
	gboolean destroying; /**< A flag that states if the session is being destroyed. */
//	gboolean http_method;

	NateonNotification *notification;
//	NateonNexus *nexus;
	NateonSync *sync;

	NateonUserList *userlist;

	int servconns_count; /**< The count of server connections. */
	GList *switches; /**< The list of all the switchboards. */
//	GList *directconns; /**< The list of all the directconnections. */
	GList *slplinks; /**< The list of all the slplinks. */

	int conv_seq; /**< The current conversation sequence number. */

	char *ticket;
//	struct
//	{
//		char *kv;
//		char *sid;
//		char *mspauth;
//		unsigned long sl;
//		char *file;
//		char *client_ip;
//		int client_port;
//
//	} passport_info;
};

/**
 * Creates an NATEON session.
 *
 * @param account The account.
 *
 * @return The new NATEON session.
 */
NateonSession *nateon_session_new(PurpleAccount *account);

/**
 * Destroys an NATEON session.
 *
 * @param session The NATEON session to destroy.
 */
void nateon_session_destroy(NateonSession *session);

/**
 * Connects to and initiates an NATEON session.
 *
 * @param session     The NATEON session.
 * @param host        The dispatch server host.
 * @param port        The dispatch server port.
 *
 * @return @c TRUE on success, @c FALSE on failure.
 */
gboolean nateon_session_connect(NateonSession *session, const char *host, int port);

/**
 * Disconnects from an NATEON session.
 *
 * @param session The NATEON session.
 */
void nateon_session_disconnect(NateonSession *session);

 /**
 * Finds a switchboard with the given username.
 *
 * @param session The NATEON session.
 * @param username The username to search for.
 *
 * @return The switchboard, if found.
 */
NateonSwitchBoard *nateon_session_find_swboard(NateonSession *session, const char *username);

 /**
 * Finds a switchboard with the given conversation.
 *
 * @param session The NATEON session.
 * @param conv    The conversation to search for.
 *
 * @return The switchboard, if found.
 */
NateonSwitchBoard *nateon_session_find_swboard_with_conv(NateonSession *session, PurpleConversation *conv);

///**
// * Finds a switchboard with the given chat ID.
// *
// * @param session The NATEON session.
// * @param chat_id The chat ID to search for.
// *
// * @return The switchboard, if found.
// */
//NateonSwitchBoard *nateon_session_find_swboard_with_id(const NateonSession *session,
//												 int chat_id);

/**
 * Returns a switchboard to communicate with certain username.
 *
 * @param session The NATEON session.
 * @param username The username to search for.
 * @param flag The flag of the switchboard
 *
 * @return The switchboard.
 */
NateonSwitchBoard *nateon_session_get_swboard(NateonSession *session, const char *username, NateonSBFlag flag);

/**
 * Sets an error for the NATEON session.
 *
 * @param session The NATEON session.
 * @param error The error.
 * @param info Extra information.
 */
void nateon_session_set_error(NateonSession *session, NateonErrorType error,
						   const char *info);

/**
 * Sets the current step in the login proccess.
 *
 * @param session The NATEON session.
 * @param step The current step.
 */
void nateon_session_set_login_step(NateonSession *session, NateonLoginStep step);

/**
 * Finish the login proccess.
 *
 * @param session The NATEON session.
 */
void nateon_session_finish_login(NateonSession *session);

#endif /* _NATEON_SESSION_H_ */
