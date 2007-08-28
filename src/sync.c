/**
 * @file sync.c NATEON list synchronization functions
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
#include "sync.h"
//#include "state.h"

static NateonTable *cbs_table;

static void conf_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	nateon_cmdproc_send(cmdproc, "GLST", NULL);
}

static void glst_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session = cmdproc->session;

	if (cmd->param_count < 6) return;

	/* Group */
	if (!strcmp(cmd->params[3], "Y"))
	{
		NateonGroup *group;
		const char *name;
		int group_id;

		group_id = atoi(cmd->params[4]);
		name = purple_url_decode(cmd->params[5]);

		group = nateon_group_new(session->userlist, group_id, name);

		/* HACK */
		if (group_id == 0)
		{
			/* Group of ungroupped buddies */
			g_free(group->name);
			group->name = g_strdup("");
		}
		else if (purple_find_group(name) == NULL)
		{
			PurpleGroup *g = purple_group_new(name);
			purple_blist_add_group(g, NULL);
		}

	}
	/* User's group info */
	else
	{
		NateonUser *user;
		char *user_id = NULL;
		int group_id;

		user_id = cmd->params[4];
		group_id = atoi(cmd->params[5]);

		user = nateon_userlist_find_user_with_id(session->userlist, user_id);
		if (user == NULL)
		{
			user = nateon_user_new(session->userlist, "", "", user_id);
			user->group_ids = NULL;

			nateon_userlist_add_user(session->userlist, user);
		}

		user->group_ids = g_list_append(user->group_ids, GINT_TO_POINTER(group_id));
	}

	//session->sync->num_groups++;

	/* end of group list */
	if (atoi(cmd->params[1])+1 == atoi(cmd->params[2]))
	{
		nateon_cmdproc_send(cmdproc, "LIST", NULL);
	}
}

static void list_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonSession *session = cmdproc->session;
	char *user_id = NULL;
	char *account = NULL;
	char *stored  = NULL;
	char *friend  = NULL;
	int list_op;
	NateonUser *user;

	user_id  = cmd->params[5];
	account  = cmd->params[4];
	stored   = purple_url_decode(cmd->params[7]);
	friend   = cmd->params[6];
	list_op  = cmd->params[3][NATEON_LIST_FL]-'0' ? NATEON_LIST_FL_OP : 0; 
	list_op += cmd->params[3][NATEON_LIST_AL]-'0' ? NATEON_LIST_AL_OP : 0; 
	list_op += cmd->params[3][NATEON_LIST_BL]-'0' ? NATEON_LIST_BL_OP : 0; 
	list_op += cmd->params[3][NATEON_LIST_RL]-'0' ? NATEON_LIST_RL_OP : 0;

	purple_debug_info("nateon", "[%s] %s%s%s%s\n", __FUNCTION__, 
			cmd->params[3][NATEON_LIST_FL]-'0'? "FL ": "",
			cmd->params[3][NATEON_LIST_AL]-'0'? "AL ": "",
			cmd->params[3][NATEON_LIST_BL]-'0'? "BL ": "",
			cmd->params[3][NATEON_LIST_RL]-'0'? "RL": "");

	user = nateon_userlist_find_user_with_id(session->userlist, user_id);
	if (user == NULL)
	{
		user = nateon_user_new(session->userlist, account, stored, user_id);
		user->group_ids = NULL;
		nateon_userlist_add_user(session->userlist, user);
	}

	g_free(user->account_name);
	user->account_name = g_strdup(account);

	g_free(user->store_name);
	user->store_name = g_strdup(stored);

	g_free(user->friendly_name);
	user->friendly_name = g_strdup(friend);

	nateon_got_list_user(session, user, list_op, user->group_ids);

	session->sync->last_user = user;

	session->sync->num_users++;

	/* end of lists */
	if (session->sync->num_users == atoi(cmd->params[2]))
	{
		cmdproc->cbs_table = session->sync->old_cbs_table;

		nateon_session_finish_login(session);

		nateon_sync_destroy(session->sync);
		session->sync = NULL;
	}
}

//static void
//blp_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	PurpleConnection *gc = cmdproc->session->account->gc;
//	const char *list_name;
//
//	list_name = cmd->params[0];
//
//	if (!g_ascii_strcasecmp(list_name, "AL"))
//	{
//		/*
//		 * If the current setting is AL, messages from users who
//		 * are not in BL will be delivered.
//		 *
//		 * In other words, deny some.
//		 */
//		gc->account->perm_deny = PURPLE_PRIVACY_DENY_USERS;
//	}
//	else
//	{
//		/* If the current setting is BL, only messages from people
//		 * who are in the AL will be delivered.
//		 *
//		 * In other words, permit some.
//		 */
//		gc->account->perm_deny = PURPLE_PRIVACY_ALLOW_USERS;
//	}
//}
//
//static void
//prp_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session = cmdproc->session;
//	const char *type, *value;
//
//	type  = cmd->params[0];
//	value = cmd->params[1];
//
//	if (cmd->param_count == 2)
//	{
//		if (!strcmp(type, "PHH"))
//			nateon_user_set_home_phone(session->user, purple_url_decode(value));
//		else if (!strcmp(type, "PHW"))
//			nateon_user_set_work_phone(session->user, purple_url_decode(value));
//		else if (!strcmp(type, "PHM"))
//			nateon_user_set_mobile_phone(session->user, purple_url_decode(value));
//	}
//	else
//	{
//		if (!strcmp(type, "PHH"))
//			nateon_user_set_home_phone(session->user, NULL);
//		else if (!strcmp(type, "PHW"))
//			nateon_user_set_work_phone(session->user, NULL);
//		else if (!strcmp(type, "PHM"))
//			nateon_user_set_mobile_phone(session->user, NULL);
//	}
//}

//static void
//lsg_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session = cmdproc->session;
//	NateonGroup *group;
//	PurpleGroup *g;
//	const char *name;
//	int group_id;
//
//	group_id = atoi(cmd->params[0]);
//	name = purple_url_decode(cmd->params[1]);
//
//	group = nateon_group_new(session->userlist, group_id, name);
//
//	/* HACK */
//	if (group_id == 0)
//		/* Group of ungroupped buddies */
//		return;
//
//	if ((g = purple_find_group(name)) == NULL)
//	{
//		g = purple_group_new(name);
//		purple_blist_add_group(g, NULL);
//	}
//}
//
//static void
//lst_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSession *session = cmdproc->session;
//	char *passport = NULL;
//	const char *friend = NULL;
//	int list_op;
//	NateonUser *user;
//
//	passport = cmd->params[0];
//	friend   = purple_url_decode(cmd->params[1]);
//	list_op  = atoi(cmd->params[2]);
//
//	user = nateon_user_new(session->userlist, passport, friend);
//
//	nateon_userlist_add_user(session->userlist, user);
//
//	session->sync->last_user = user;
//
//	/* TODO: This can be improved */
//
//	if (list_op & NATEON_LIST_FL_OP)
//	{
//		char **c;
//		char **tokens;
//		const char *group_nums;
//		GSList *group_ids;
//
//		group_nums = cmd->params[3];
//
//		group_ids = NULL;
//
//		tokens = g_strsplit(group_nums, ",", -1);
//
//		for (c = tokens; *c != NULL; c++)
//		{
//			int id;
//
//			id = atoi(*c);
//			group_ids = g_slist_append(group_ids, GINT_TO_POINTER(id));
//		}
//
//		g_strfreev(tokens);
//
//		nateon_got_lst_user(session, user, list_op, group_ids);
//
//		g_slist_free(group_ids);
//	}
//	else
//	{
//		nateon_got_lst_user(session, user, list_op, NULL);
//	}
//
//	session->sync->num_users++;
//
//	if (session->sync->num_users == session->sync->total_users)
//	{
//		cmdproc->cbs_table = session->sync->old_cbs_table;
//
//		nateon_session_finish_login(session);
//
//		nateon_sync_destroy(session->sync);
//		session->sync = NULL;
//	}
//}
//
//static void
//bpr_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
//{
//	NateonSync *sync = cmdproc->session->sync;
//	const char *type, *value;
//	NateonUser *user;
//
//	user = sync->last_user;
//
//	type     = cmd->params[0];
//	value    = cmd->params[1];
//
//	if (value)
//	{
//		if (!strcmp(type, "MOB"))
//		{
//			if (!strcmp(value, "Y"))
//				user->mobile = TRUE;
//		}
//		else if (!strcmp(type, "PHH"))
//			nateon_user_set_home_phone(user, purple_url_decode(value));
//		else if (!strcmp(type, "PHW"))
//			nateon_user_set_work_phone(user, purple_url_decode(value));
//		else if (!strcmp(type, "PHM"))
//			nateon_user_set_mobile_phone(user, purple_url_decode(value));
//	}
//}

void
nateon_sync_init(void)
{
	/* TODO: check prp, blp, bpr */

	cbs_table = nateon_table_new();

	/* Syncing */
	nateon_table_add_cmd(cbs_table, "CONF", "CONF", conf_cmd);
	nateon_table_add_cmd(cbs_table, "GLST", "GLST", glst_cmd);
	nateon_table_add_cmd(cbs_table, "LIST", "LIST", list_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "GTC", NULL);
//	nateon_table_add_cmd(cbs_table, NULL, "BLP", blp_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "PRP", prp_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "LSG", lsg_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "LST", lst_cmd);
//	nateon_table_add_cmd(cbs_table, NULL, "BPR", bpr_cmd);
}

void
nateon_sync_end(void)
{
	nateon_table_destroy(cbs_table);
}

NateonSync *
nateon_sync_new(NateonSession *session)
{
	NateonSync *sync;

	sync = g_new0(NateonSync, 1);

	sync->session = session;
	sync->cbs_table = cbs_table;

	return sync;
}

void
nateon_sync_destroy(NateonSync *sync)
{
	g_free(sync);
}
