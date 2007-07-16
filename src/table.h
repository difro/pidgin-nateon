/**
 * @file table.h NATEON helper structure
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
#ifndef _NATEON_TABLE_H_
#define _NATEON_TABLE_H_

typedef struct _NateonTable NateonTable;

#include "cmdproc.h"
#include "transaction.h"
#include "msg.h"

//typedef void (*NateonMsgTypeCb)(NateonCmdProc *cmdproc, NateonMessage *msg);

struct _NateonTable
{
	GHashTable *cmds;
//	GHashTable *msgs;
//	GHashTable *errors;
//
	GHashTable *async;
//	GHashTable *fallback;
};

NateonTable *nateon_table_new(void);
void nateon_table_destroy(NateonTable *table);

void nateon_table_add_cmd(NateonTable *table, char *command, char *answer, NateonTransCb cb);
//void nateon_table_add_error(NateonTable *table, char *answer, NateonErrorCb cb);
//void nateon_table_add_msg_type(NateonTable *table, char *type, NateonMsgTypeCb cb);
//
#endif /* _NATEON_TABLE_H_ */
