/**
 * @file notification.h Notification server functions
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
#ifndef _NATEON_NOTIFICATION_H_
#define _NATEON_NOTIFICATION_H_

typedef struct _NateonNotification NateonNotification;

#include "session.h"
#include "servconn.h"
#include "cmdproc.h"

struct _NateonNotification
{
	NateonSession *session;
	NateonCmdProc *cmdproc;
	NateonServConn *servconn;

	gboolean in_use;
};

#include "state.h"

void nateon_notification_end(void);
void nateon_notification_init(void);

void nateon_notification_add_buddy(NateonNotification *notification, const char *list, const char *who, const char *user_id, int group_id);
void nateon_notification_rem_buddy(NateonNotification *notification, const char *list, const char *who, int group_id, const char *account);
void nateon_notification_move_buddy(NateonNotification *notification, const char *who, const char *user_id, int old_group_id, int new_group_id);
void nateon_notification_copy_buddy(NateonNotification *notification, const char *who, const char *user_id, int old_group_id, int new_group_id);
NateonNotification *nateon_notification_new(NateonSession *session);
void nateon_notification_destroy(NateonNotification *notification);
gboolean nateon_notification_connect(NateonNotification *notification,
							  const char *host, int port);
void nateon_notification_disconnect(NateonNotification *notification);

/**
 * Closes a notification.
 *
 * It's first closed, and then disconnected.
 * 
 * @param notification The notification object to close.
 */
void nateon_notification_close(NateonNotification *notification);

//void nateon_got_login_params(NateonSession *session, const char *login_params);

#endif /* _NATEON_NOTIFICATION_H_ */
