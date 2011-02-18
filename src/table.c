/**
 * @file table.c NATEON helper structure
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
#include "table.h"

static void
null_cmd_cb(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
}

static void
null_error_cb(NateonCmdProc *cmdproc, NateonTransaction *trans, int error)
{
}

NateonTable *
nateon_table_new()
{
	NateonTable *table;

	table = g_new0(NateonTable, 1);

	table->cmds = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
//	table->msgs = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
	table->errors = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

	table->async = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
//	table->fallback = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);

	return table;
}

void
nateon_table_destroy(NateonTable *table)
{
	g_return_if_fail(table != NULL);

	g_hash_table_destroy(table->cmds);
//	g_hash_table_destroy(table->msgs);
	g_hash_table_destroy(table->errors);

	g_hash_table_destroy(table->async);
//	g_hash_table_destroy(table->fallback);

	g_free(table);
}

void
nateon_table_add_cmd(NateonTable *table,
				  char *command, char *answer, NateonTransCb cb)
{
	GHashTable *cbs;

	g_return_if_fail(table  != NULL);
	g_return_if_fail(answer != NULL);

	cbs = NULL;

	if (command == NULL)
	{
		cbs = table->async;
	}
//	else if (strcmp(command, "fallback") == 0)
//	{
//		cbs = table->fallback;
//	}
	else
	{
		cbs = g_hash_table_lookup(table->cmds, command);

		if (cbs == NULL)
		{
			cbs = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);
			g_hash_table_insert(table->cmds, command, cbs);
		}
		else
		{
			purple_debug_error( "nateon", "Hash table collision!\n" );
			g_assert( 0 );
		}
	}

	if (cb == NULL)
		cb = null_cmd_cb;

	g_hash_table_insert(cbs, answer, cb);
}

void
nateon_table_add_error(NateonTable *table,
					char *answer, NateonErrorCb cb)
{
	g_return_if_fail(table  != NULL);
	g_return_if_fail(answer != NULL);

	if (cb == NULL)
		cb = null_error_cb;

	g_hash_table_insert(table->errors, answer, cb);
}

//void
//nateon_table_add_msg_type(NateonTable *table,
//					   char *type, NateonMsgTypeCb cb)
//{
//	g_return_if_fail(table != NULL);
//	g_return_if_fail(type  != NULL);
//	g_return_if_fail(cb    != NULL);
//
//#if 0
//	if (cb == NULL)
//		cb = null_msg_cb;
//#endif
//
//	g_hash_table_insert(table->msgs, type, cb);
//}
