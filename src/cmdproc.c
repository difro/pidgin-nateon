/**
 * @file cmdproc.c NATEON command processor functions
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
#include "cmdproc.h"

NateonCmdProc *
nateon_cmdproc_new(NateonSession *session)
{
	NateonCmdProc *cmdproc;

	cmdproc = g_new0(NateonCmdProc, 1);

	cmdproc->session = session;
	cmdproc->txqueue = g_queue_new();
	cmdproc->history = nateon_history_new();

	return cmdproc;
}

void
nateon_cmdproc_destroy(NateonCmdProc *cmdproc)
{
	NateonTransaction *trans;

//	while ((trans = g_queue_pop_head(cmdproc->txqueue)) != NULL)
//		nateon_transaction_destroy(trans);

	g_queue_free(cmdproc->txqueue);

	nateon_history_destroy(cmdproc->history);

	if (cmdproc->last_cmd != NULL)
		nateon_command_destroy(cmdproc->last_cmd);

	g_free(cmdproc);
}

void
nateon_cmdproc_process_queue(NateonCmdProc *cmdproc)
{
	NateonTransaction *trans;

	while ((trans = g_queue_pop_head(cmdproc->txqueue)) != NULL)
		nateon_cmdproc_send_trans(cmdproc, trans);
}

void
nateon_cmdproc_queue_trans(NateonCmdProc *cmdproc, NateonTransaction *trans)
{
	g_return_if_fail(cmdproc != NULL);
	g_return_if_fail(trans   != NULL);

	g_queue_push_tail(cmdproc->txqueue, trans);
}

static void
show_debug_cmd(NateonCmdProc *cmdproc, gboolean incoming, const char *command)
{
	NateonServConn *servconn;
	const char *names[] = { "NS", "SB" };
	char *show;
	char tmp;
	size_t len;

	servconn = cmdproc->servconn;
	len = strlen(command);
	show = g_strdup(command);

	tmp = (incoming) ? 'S' : 'C';

	if ((show[len - 1] == '\n') && (show[len - 2] == '\r'))
	{
		show[len - 2] = '\0';
	}

	purple_debug_misc("nateon", "%c: %s %03d: %s\n", tmp,
					names[servconn->type], servconn->num, show);

	g_free(show);
}

void
nateon_cmdproc_send_trans(NateonCmdProc *cmdproc, NateonTransaction *trans)
{
	NateonServConn *servconn;
	char *data;
	size_t len;

	g_return_if_fail(cmdproc != NULL);
	g_return_if_fail(trans != NULL);

	servconn = cmdproc->servconn;

	if (!servconn->connected)
		return;

	nateon_history_add(cmdproc->history, trans);

	data = nateon_transaction_to_string(trans);

	len = strlen(data);

	show_debug_cmd(cmdproc, FALSE, data);

	if (trans->callbacks == NULL)
		trans->callbacks = g_hash_table_lookup(cmdproc->cbs_table->cmds, trans->command);

	if (trans->payload != NULL)
	{
		data = g_realloc(data, len + trans->payload_len);
		memcpy(data + len, trans->payload, trans->payload_len);
		len += trans->payload_len;
	}

	nateon_servconn_write(servconn, data, len);

	g_free(data);
}

//void
//nateon_cmdproc_send_quick(NateonCmdProc *cmdproc, const char *command,
//					   const char *format, ...)
//{
//	NateonServConn *servconn;
//	char *data;
//	char *params = NULL;
//	va_list arg;
//	size_t len;
//
//	g_return_if_fail(cmdproc != NULL);
//	g_return_if_fail(command != NULL);
//
//	servconn = cmdproc->servconn;
//
//	if (!servconn->connected)
//		return;
//
//	if (format != NULL)
//	{
//		va_start(arg, format);
//		params = g_strdup_vprintf(format, arg);
//		va_end(arg);
//	}
//
//	if (params != NULL)
//		data = g_strdup_printf("%s %s\r\n", command, params);
//	else
//		data = g_strdup_printf("%s\r\n", command);
//
//	g_free(params);
//
//	len = strlen(data);
//
//	show_debug_cmd(cmdproc, FALSE, data);
//
//	nateon_servconn_write(servconn, data, len);
//
//	g_free(data);
//}

void
nateon_cmdproc_send(NateonCmdProc *cmdproc, const char *command,
				 const char *format, ...)
{
	NateonTransaction *trans;
	va_list arg;

	g_return_if_fail(cmdproc != NULL);
	g_return_if_fail(command != NULL);

	if (!cmdproc->servconn->connected)
		return;

	trans = g_new0(NateonTransaction, 1);

	trans->command = g_strdup(command);

	if (format != NULL)
	{
		va_start(arg, format);
		trans->params = g_strdup_vprintf(format, arg);
		va_end(arg);
	}

	nateon_cmdproc_send_trans(cmdproc, trans);
}

//void
//nateon_cmdproc_process_payload(NateonCmdProc *cmdproc, char *payload,
//							int payload_len)
//{
//	NateonCommand *last;
//
//	g_return_if_fail(cmdproc != NULL);
//
//	last = cmdproc->last_cmd;
//	last->payload = g_memdup(payload, payload_len);
//	last->payload_len = payload_len;
//
//	if (last->payload_cb != NULL)
//		last->payload_cb(cmdproc, last, payload, payload_len);
//}
//
//void
//nateon_cmdproc_process_msg(NateonCmdProc *cmdproc, NateonMessage *msg)
//{
//	NateonMsgTypeCb cb;
//
//	if (nateon_message_get_content_type(msg) == NULL)
//	{
//		purple_debug_misc("nateon", "failed to find message content\n");
//		return;
//	}
//
//	cb = g_hash_table_lookup(cmdproc->cbs_table->msgs,
//							 nateon_message_get_content_type(msg));
//
//	if (cb == NULL)
//	{
//		purple_debug_warning("nateon", "Unhandled content-type '%s'\n",
//						   nateon_message_get_content_type(msg));
//
//		return;
//	}
//
//	cb(cmdproc, msg);
//}

void
nateon_cmdproc_process_cmd(NateonCmdProc *cmdproc, NateonCommand *cmd)
{
	NateonTransCb cb = NULL;
	NateonTransaction *trans = NULL;

	if (cmd->trId)
		trans = nateon_history_find(cmdproc->history, cmd->trId);

	if (trans != NULL)
		if (trans->timer)
			purple_timeout_remove(trans->timer);

	if (g_ascii_isdigit(cmd->command[0]))
	{
		if (trans != NULL)
		{
			NateonErrorCb error_cb = NULL;
			int error;

			error = atoi(cmd->command);
			purple_debug_info("nateon", "[%s] error(%d)\n", __FUNCTION__, error);

			if (trans->error_cb != NULL)
				error_cb = trans->error_cb;

			if (error_cb == NULL && cmdproc->cbs_table->errors != NULL)
				error_cb = g_hash_table_lookup(cmdproc->cbs_table->errors, trans->command);

			if (error_cb != NULL)
			{
				purple_debug_info("nateon", "[%s] 에러콜백있음\n", __FUNCTION__);
				error_cb(cmdproc, trans, error);
			}
			else
			{
				purple_debug_info("nateon", "[%s] 에러콜백없음\n", __FUNCTION__);
#if 1
				nateon_error_handle(cmdproc->session, error);
#else
				purple_debug_warning("nateon", "Unhandled error '%s'\n",
								   cmd->command);
#endif
			}

			return;
		}
	}

	if (cmdproc->cbs_table->async != NULL)
		cb = g_hash_table_lookup(cmdproc->cbs_table->async, cmd->command);

	if (cb == NULL && trans != NULL)
	{
		cmd->trans = trans;

		if (trans->callbacks != NULL)
			cb = g_hash_table_lookup(trans->callbacks, cmd->command);
	}
//
//	if (cb == NULL && cmdproc->cbs_table->fallback != NULL)
//		cb = g_hash_table_lookup(cmdproc->cbs_table->fallback, cmd->command);

	if (cb != NULL)
	{
		cb(cmdproc, cmd);
	}
	else
	{
		purple_debug_warning("nateon", "Unhandled command '%s'\n",
						   cmd->command);
	}

//	if (trans != NULL && trans->pendent_cmd != NULL)
//		nateon_transaction_unqueue_cmd(trans, cmdproc);
}

void
nateon_cmdproc_process_cmd_text(NateonCmdProc *cmdproc, const char *command)
{
	show_debug_cmd(cmdproc, TRUE, command);

	if (cmdproc->last_cmd != NULL)
		nateon_command_destroy(cmdproc->last_cmd);

	cmdproc->last_cmd = nateon_command_from_string(command);

	nateon_cmdproc_process_cmd(cmdproc, cmdproc->last_cmd);
}
