/**
 * @file transaction.h NATEON transaction functions
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
#ifndef _NATEON_TRANSACTION_H
#define _NATEON_TRANSACTION_H

typedef struct _NateonTransaction NateonTransaction;

#include "command.h"
#include "cmdproc.h"

typedef void (*NateonTransCb)(NateonCmdProc *cmdproc, NateonCommand *cmd);
//typedef void (*NateonTimeoutCb)(NateonCmdProc *cmdproc, NateonTransaction *trans);
//typedef void (*NateonErrorCb)(NateonCmdProc *cmdproc, NateonTransaction *trans,
//						   int error);

/**
 * A transaction. A sending command that will initiate the transaction.
 */
struct _NateonTransaction
{
	NateonCmdProc *cmdproc;
	unsigned int trId;

	char *command;
	char *params;

	int timer;

	void *data; /**< The data to be used on the different callbacks. */
	GHashTable *callbacks;
	gboolean has_custom_callbacks;
//	NateonErrorCb error_cb;
//	NateonTimeoutCb timeout_cb;
//
	char *payload;
	size_t payload_len;
//
//	GQueue *queue;
//	NateonCommand *pendent_cmd; /**< The command that is waiting for the result of
//							   this transaction. */
};

NateonTransaction *nateon_transaction_new(NateonCmdProc *cmdproc, const char *command, const char *format, ...);
//void nateon_transaction_destroy(NateonTransaction *trans);

char *nateon_transaction_to_string(NateonTransaction *trans);
//void nateon_transaction_queue_cmd(NateonTransaction *trans, NateonCommand *cmd);
//void nateon_transaction_unqueue_cmd(NateonTransaction *trans, NateonCmdProc *cmdproc);
//void nateon_transaction_set_payload(NateonTransaction *trans,
//								 const char *payload, int payload_len);
void nateon_transaction_set_data(NateonTransaction *trans, void *data);
void nateon_transaction_add_cb(NateonTransaction *trans, char *answer, NateonTransCb cb);
//void nateon_transaction_set_error_cb(NateonTransaction *trans, NateonErrorCb cb);
//void nateon_transaction_set_timeout_cb(NateonTransaction *trans, NateonTimeoutCb cb);

#endif /* _NATEON_TRANSACTION_H */
