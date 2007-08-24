/**
 * @file cmdproc.h NATEON command processor functions
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
#ifndef _NATEON_CMDPROC_H_
#define _NATEON_CMDPROC_H_

typedef struct _NateonCmdProc NateonCmdProc;

#include "session.h"
#include "servconn.h"
#include "error.h"
#include "command.h"
#include "table.h"
#include "history.h"

struct _NateonCmdProc
{
	NateonSession *session;
	NateonServConn *servconn;

	GQueue *txqueue;

	NateonCommand *last_cmd;

	NateonTable *cbs_table;

	NateonHistory *history;

	void *data; /**< Extra data, like the switchboard. */
};

NateonCmdProc *nateon_cmdproc_new(NateonSession *session);
void nateon_cmdproc_destroy(NateonCmdProc *cmdproc);

void nateon_cmdproc_process_queue(NateonCmdProc *cmdproc);

void nateon_cmdproc_send_trans(NateonCmdProc *cmdproc, NateonTransaction *trans);
void nateon_cmdproc_queue_trans(NateonCmdProc *cmdproc, NateonTransaction *trans);
void nateon_cmdproc_send(NateonCmdProc *cmdproc, const char *command,
					  const char *format, ...);
//void nateon_cmdproc_send_quick(NateonCmdProc *cmdproc, const char *command,
//							const char *format, ...);

//void nateon_cmdproc_process_msg(NateonCmdProc *cmdproc,
//							 NateonMessage *msg);
void nateon_cmdproc_process_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd);
void nateon_cmdproc_process_cmd_text(NateonCmdProc *cmdproc, const char *command);
//void nateon_cmdproc_process_payload(NateonCmdProc *cmdproc,
//								 char *payload, int payload_len);
//
//void nateon_cmdproc_disconnect(NateonCmdProc *cmdproc);

#endif /* _NATEON_CMDPROC_H_ */
