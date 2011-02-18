/**
 * @file command.c NATEON command functions
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
#include "command.h"

static gboolean
is_num(char *str)
{
	char *c;
	for (c = str; *c; c++) {
		if (!(g_ascii_isdigit(*c)))
			return FALSE;
	}

	return TRUE;
}

/**
 * Creates a new command from CMD+binary data payload.
 * e.g, EMFL	NCE20110221112917.bmp	1498	:_	BM....
 *
 * @param sz     Length of data.
 * @param data   Binary data. Note that data will NOT copied but will be referenced
 *               by the new output NateonCommand.
 *               The data will be destroyed when command is destroyed by
 *               nateon_command_destroy.
 */
NateonCommand *
nateon_command_from_binary(char *data, int sz)
{
	int i;
	NateonCommand *cmd;

	g_return_val_if_fail(data != NULL, NULL);

	cmd = g_new0(NateonCommand, 1);

	// find the first tab character
	for( i = 0 ; i < sz && data[i] != '\t' ; ++i );
	g_assert( i < sz && data[i] == '\t' ); // we are expecting at least one tab char.

	// from data[0] to data[i-1], it is the command string.
	cmd->command = g_strndup(data, i);
	//printf("new command from binary: \"%s\\""\n", cmd->command);

	cmd->trId = 0; // might not have trId. for EMFL, certainly not.

	// We'll leave parameter parsing as each handler's job,
	// since we might have '\t' in binary data.
	cmd->bin_data_sz = sz;
	cmd->bin_data = data;

	nateon_command_ref(cmd);

	return cmd;
}

NateonCommand *
nateon_command_from_string(const char *string)
{
	NateonCommand *cmd;
	char *tmp;
	char *param_start;

	g_return_val_if_fail(string != NULL, NULL);

	tmp = g_strdup(string);
	param_start = strchr(tmp, ' ');

	cmd = g_new0(NateonCommand, 1);
	cmd->command = tmp;

	if (param_start)
	{
		char *param;
		int c;

		*param_start++ = '\0';
		cmd->params = g_strsplit(param_start, " ", 0);

		for (c = 0; cmd->params[c]; c++);
		cmd->param_count = c;

		param = cmd->params[0];

		cmd->trId = is_num(param) ? atoi(param) : 0;
	}
	else
		cmd->trId = 0;

	nateon_command_ref(cmd);

	return cmd;
}

void
nateon_command_destroy(NateonCommand *cmd)
{
	g_return_if_fail(cmd != NULL);

	if (cmd->ref_count > 0)
	{
		nateon_command_unref(cmd);
		return;
	}

	if (cmd->payload != NULL)
		g_free(cmd->payload);

	if (cmd->bin_data != NULL)
		g_free(cmd->bin_data);

	g_free(cmd->command);
	g_strfreev(cmd->params);
	g_free(cmd);
}

NateonCommand *
nateon_command_ref(NateonCommand *cmd)
{
	g_return_val_if_fail(cmd != NULL, NULL);

	cmd->ref_count++;
	return cmd;
}

NateonCommand *
nateon_command_unref(NateonCommand *cmd)
{
	g_return_val_if_fail(cmd != NULL, NULL);
	g_return_val_if_fail(cmd->ref_count > 0, NULL);

	cmd->ref_count--;

	if (cmd->ref_count == 0)
	{
		nateon_command_destroy(cmd);
		return NULL;
	}

	return cmd;
}
