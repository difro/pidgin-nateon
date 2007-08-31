/**
 * @file state.c State functions and definitions
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
#include "state.h"

//static const char *away_text[] =
//{
//	N_("Available"),
//	N_("Available"),
//	N_("Busy"),
//	N_("Idle"),
//	N_("Be Right Back"),
//	N_("Away From Computer"),
//	N_("On The Phone"),
//	N_("Out To Lunch"),
//	N_("Available"),
//	N_("Available")
//};

void
nateon_change_status(NateonSession *session)
{
	PurpleAccount *account;
	NateonCmdProc *cmdproc;
//	NateonUser *user;
//	NateonObject *nateonobj;
	const char *state_text;

	g_return_if_fail(session != NULL);
	g_return_if_fail(session->notification != NULL);

	account = session->account;
	cmdproc = session->notification->cmdproc;
//	user = session->user;
	state_text = nateon_state_get_text(nateon_state_from_account(account));

	/* If we're not logged in yet, don't send the status to the server,
	 * it will be sent when login completes
	 */
	if (!session->logged_in)
		return;

	nateon_cmdproc_send(cmdproc, "ONST", "%s 0 %%00 1", state_text);

//	nateonobj = nateon_user_get_object(user);

//	if (nateonobj == NULL)
//	{
//		nateon_cmdproc_send(cmdproc, "CHG", "%s %d", state_text,
//						 NATEON_CLIENT_ID);
//	}
//	else
//	{
//		char *nateonobj_str;
//
//		nateonobj_str = nateon_object_to_string(nateonobj);
//
//		nateon_cmdproc_send(cmdproc, "CHG", "%s %d %s", state_text,
//						 NATEON_CLIENT_ID, purple_url_encode(nateonobj_str));
//
//		g_free(nateonobj_str);
//	}
}

//const char *
//nateon_away_get_text(NateonAwayType type)
//{
//	g_return_val_if_fail(type <= NATEON_HIDDEN, NULL);
//
//	return _(away_text[type]);
//}

const char *
nateon_state_get_text(NateonAwayType state)
{
	static char *status_text[] =
	{ "O", "A", "B", "P", "M", "F" };

	return status_text[state];
}

NateonAwayType
nateon_state_from_account(PurpleAccount *account)
{
	NateonAwayType nateonstatus;
	PurplePresence *presence;
	PurpleStatus *status;
	const char *status_id;

	purple_debug_info("nateon", "[%s] have some bugs.\n", __FUNCTION__);

	presence = purple_account_get_presence(account);
	status = purple_presence_get_active_status(presence);
	status_id = purple_status_get_id(status);

	purple_debug_info("nateon", "[%s] status_id(%s)\n", __FUNCTION__, status_id);

	if (!strcmp(status_id, "A"))
		nateonstatus = NATEON_AWAY;
	else if (!strcmp(status_id, "B"))
		nateonstatus = NATEON_BUSY;
	else if (!strcmp(status_id, "P"))
		nateonstatus = NATEON_PHONE;
	else if (!strcmp(status_id, "M"))
		nateonstatus = NATEON_MEETING;
	else
		nateonstatus = NATEON_ONLINE;

	purple_debug_info("nateon", "[%s] nateonstatus(%s)\n", __FUNCTION__, nateon_state_get_text(nateonstatus));

//	if ((nateonstatus == NATEON_ONLINE) && purple_presence_is_idle(presence))
//		nateonstatus = NATEON_IDLE;

	purple_debug_info("nateon", "[%s] nateonstatus(%s)\n", __FUNCTION__, nateon_state_get_text(nateonstatus));

	return nateonstatus;
}
