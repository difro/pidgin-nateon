/**
 * @file command.h NATEON command functions
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
#ifndef _NATEON_COMMAND_H
#define _NATEON_COMMAND_H

typedef struct _NateonCommand NateonCommand;

#include "cmdproc.h"
#include "transaction.h"

typedef void (*NateonPayloadCb)(NateonCmdProc *cmdproc, NateonCommand *cmd, char *payload, size_t len);

/**
 * A received command.
 */
struct _NateonCommand
{
	unsigned int trId;

	char *command;
	char **params;
	int param_count;

	int ref_count;

	NateonTransaction *trans;

	char *payload;
	size_t payload_len;

	NateonPayloadCb payload_cb;
};

NateonCommand *nateon_command_from_string(const char *string);
void nateon_command_destroy(NateonCommand *cmd);
NateonCommand *nateon_command_ref(NateonCommand *cmd);
NateonCommand *nateon_command_unref(NateonCommand *cmd);

#endif /* _NATEON_COMMAND_H */
