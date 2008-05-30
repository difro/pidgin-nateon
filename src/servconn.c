/**
 * @file servconn.c Server connection functions
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
#include "servconn.h"
#include "error.h"

static void read_cb(gpointer data, gint source, PurpleInputCondition cond);

/**************************************************************************
 * Main
 **************************************************************************/

NateonServConn *
nateon_servconn_new(NateonSession *session, NateonServConnType type)
{
	NateonServConn *servconn;

	g_return_val_if_fail(session != NULL, NULL);

	servconn = g_new0(NateonServConn, 1);

	servconn->type = type;

	servconn->session = session;
	servconn->cmdproc = nateon_cmdproc_new(session);
	servconn->cmdproc->servconn = servconn;

//	servconn->httpconn = nateon_httpconn_new(servconn);

	servconn->num = session->servconns_count++;

	servconn->tx_buf = purple_circ_buffer_new(NATEON_BUF_LEN);
	servconn->tx_handler = -1;

	return servconn;
}

void
nateon_servconn_destroy(NateonServConn *servconn)
{
	g_return_if_fail(servconn != NULL);

	if (servconn->processing)
	{
		servconn->wasted = TRUE;
		return;
	}

	if (servconn->connected)
		nateon_servconn_disconnect(servconn);

	if (servconn->destroy_cb)
		servconn->destroy_cb(servconn);

//	if (servconn->httpconn != NULL)
//		nateon_httpconn_destroy(servconn->httpconn);

	g_free(servconn->host);

	purple_circ_buffer_destroy(servconn->tx_buf);
	if (servconn->tx_handler > 0)
		purple_input_remove(servconn->tx_handler);

	nateon_cmdproc_destroy(servconn->cmdproc);
	g_free(servconn);
}

void nateon_servconn_set_connect_cb(NateonServConn *servconn, void (*connect_cb)(NateonServConn *))
{
	g_return_if_fail(servconn != NULL);
	servconn->connect_cb = connect_cb;
}

void nateon_servconn_set_disconnect_cb(NateonServConn *servconn, void (*disconnect_cb)(NateonServConn *))
{
	g_return_if_fail(servconn != NULL);

	servconn->disconnect_cb = disconnect_cb;
}

void nateon_servconn_set_destroy_cb(NateonServConn *servconn, void (*destroy_cb)(NateonServConn *))
{
	g_return_if_fail(servconn != NULL);

	servconn->destroy_cb = destroy_cb;
}

/**************************************************************************
 * Utility
 **************************************************************************/

void
nateon_servconn_got_error(NateonServConn *servconn, NateonServConnError error)
{
	char *tmp;
	const char *reason;

	const char *names[] = { "Notification", "Switchboard" };
	const char *name;

	name = names[servconn->type];

	switch (error)
	{
		case NATEON_SERVCONN_ERROR_CONNECT:
			reason = _("Unable to connect"); break;
		case NATEON_SERVCONN_ERROR_WRITE:
			reason = _("Writing error"); break;
		case NATEON_SERVCONN_ERROR_READ:
			reason = _("Reading error"); break;
		default:
			reason = _("Unknown error"); break;
	}

	purple_debug_error("nateon", "Connection error from %s server (%s): %s\n",
					 name, servconn->host, reason);
	tmp = g_strdup_printf(_("Connection error from %s server:\n%s"),
						  name, reason);

	if (servconn->type == NATEON_SERVCONN_NS)
	{
		nateon_session_set_error(servconn->session, NATEON_ERROR_SERVCONN, tmp);
	}
	else if (servconn->type == NATEON_SERVCONN_SB)
	{
		NateonSwitchBoard *swboard;
		swboard = servconn->cmdproc->data;
		if (swboard != NULL)
			swboard->error = NATEON_SB_ERROR_CONNECTION;
	}

	nateon_servconn_disconnect(servconn);

	g_free(tmp);
}

/**************************************************************************
 * Connect
 **************************************************************************/

static void
connect_cb(gpointer data, gint source, const gchar *error_message)
{
	NateonServConn *servconn = data;
	servconn->connect_data = NULL;
	servconn->processing = FALSE;

	if (servconn->wasted)
	{
		nateon_servconn_destroy(servconn);
		return;
	}

	servconn->fd = source;

	if (source > 0)
	{
		NateonSession *session = servconn->session;
		
		servconn->connected = TRUE;

		/* Someone wants to know we connected. */
		if (session->prs_method)
		{
			NateonCmdProc *cmdproc = servconn->cmdproc;
			nateon_cmdproc_send(cmdproc, "RCON", "%s %d", servconn->host, 5004); 
		}
		else 
		{
			servconn->connect_cb(servconn);
		}
		servconn->inpa = purple_input_add(servconn->fd, PURPLE_INPUT_READ,
			read_cb, data);
	}
	else
	{
		nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_CONNECT);
	}
}

gboolean
nateon_servconn_connect(NateonServConn *servconn, const char *host, int port)
{
	NateonSession *session;

	g_return_val_if_fail(servconn != NULL, FALSE);
	g_return_val_if_fail(host     != NULL, FALSE);
	g_return_val_if_fail(port      > 0,    FALSE);

	session = servconn->session;

	if (servconn->connected)
		nateon_servconn_disconnect(servconn);

	if (servconn->host != NULL)
		g_free(servconn->host);

	servconn->host = g_strdup(host);

//	if (session->http_method)
//	{
//		/* HTTP Connection. */
//
//		if (!servconn->httpconn->connected)
//			if (!nateon_httpconn_connect(servconn->httpconn, host, port))
//				return FALSE;;
//
//		servconn->connected = TRUE;
//		servconn->httpconn->virgin = TRUE;
//
//		/* Someone wants to know we connected. */
//		servconn->connect_cb(servconn);
//
//		return TRUE;
//	}

	if (session->prs_method)
	{
		host = purple_account_get_string(session->account, "prs_method_server", NATEON_PRS_SERVER); 
		port = 80;
	}

	servconn->connect_data = purple_proxy_connect(NULL, session->account, host, port, connect_cb,
		servconn);

	if (servconn->connect_data != NULL)
	{
		servconn->processing = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}

void
nateon_servconn_disconnect(NateonServConn *servconn)
{
	g_return_if_fail(servconn != NULL);

	if (!servconn->connected)
	{
		purple_debug_info("nateon", "We could not connect.\n");

		/* We could not connect. */
		if (servconn->disconnect_cb != NULL)
			servconn->disconnect_cb(servconn);

		return;
	}

//	if (servconn->session->http_method)
//	{
//		/* Fake disconnection. */
//		if (servconn->disconnect_cb != NULL)
//			servconn->disconnect_cb(servconn);
//
//		return;
//	}

	if (servconn->connect_data != NULL)
	{
		purple_proxy_connect_cancel(servconn->connect_data);
		servconn->connect_data = NULL;
	}

	if (servconn->inpa > 0)
	{
		purple_input_remove(servconn->inpa);
		servconn->inpa = 0;
	}

	close(servconn->fd);

	servconn->rx_buf = NULL;
	servconn->rx_len = 0;
	servconn->payload_len = 0;

	servconn->connected = FALSE;

	if (servconn->disconnect_cb != NULL)
		servconn->disconnect_cb(servconn);
}

static void
servconn_write_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	NateonServConn *servconn = data;
	int ret, writelen;

	purple_debug_info("nateon", "write_cb\n");
	writelen = purple_circ_buffer_get_max_read(servconn->tx_buf);

	if (writelen == 0) {
		purple_input_remove(servconn->tx_handler);
		servconn->tx_handler = -1;
		return;
	}

	ret = write(servconn->fd, servconn->tx_buf->outptr, writelen);

	if (ret < 0 && errno == EAGAIN)
		return;
	else if (ret <= 0) {
		nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_WRITE);
		return;
	}

	purple_circ_buffer_mark_read(servconn->tx_buf, ret);
}

ssize_t
nateon_servconn_write(NateonServConn *servconn, const char *buf, size_t len)
{
	ssize_t ret = 0;

	g_return_val_if_fail(servconn != NULL, 0);


//	if (!servconn->session->http_method)
//	{
		if (servconn->tx_handler == -1) {
			switch (servconn->type)
			{
				case NATEON_SERVCONN_NS:
				case NATEON_SERVCONN_SB:
					ret = write(servconn->fd, buf, len);
					break;
#if 0
				case NATEON_SERVCONN_DC:
					ret = write(servconn->fd, &buf, sizeof(len));
					ret = write(servconn->fd, buf, len);
					break;
#endif
				default:
					ret = write(servconn->fd, buf, len);
					break;
			}
		} else {
			ret = -1;
			errno = EAGAIN;
		}

		if (ret < 0 && errno == EAGAIN)
			ret = 0;
		if (ret < len) {
			if (servconn->tx_handler == -1)
				servconn->tx_handler = purple_input_add(
					servconn->fd, PURPLE_INPUT_WRITE,
					servconn_write_cb, servconn);
			purple_circ_buffer_append(servconn->tx_buf, buf + ret,
				len - ret);
		}
//	}
//	else
//	{
//		ret = nateon_httpconn_write(servconn->httpconn, buf, len);
//	}

	if (ret == -1)
	{
		nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_WRITE);
	}

	return ret;
}

static void
read_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	NateonServConn *servconn;
	NateonSession *session;
	char buf[NATEON_BUF_LEN];
	char *cur, *end, *old_rx_buf;
	int len, cur_len;

	servconn = data;
	session = servconn->session;

	len = read(servconn->fd, buf, sizeof(buf) - 1);

	if (len < 0 && errno == EAGAIN)
		return;
	else if (len <= 0)
	{
		purple_debug_error("nateon", "servconn read error, len: %d error: %s\n", len, strerror(errno));
		nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_READ);
		return;
	}

	buf[len] = '\0';

	servconn->rx_buf = g_realloc(servconn->rx_buf, len + servconn->rx_len + 1);
	memcpy(servconn->rx_buf + servconn->rx_len, buf, len + 1);
	servconn->rx_len += len;

	end = old_rx_buf = servconn->rx_buf;

	servconn->processing = TRUE;

	do
	{
		cur = end;

		if (servconn->payload_len)
		{
			if (servconn->payload_len > servconn->rx_len)
				/* The payload is still not complete. */
				break;

			cur_len = servconn->payload_len;
			end += cur_len;
		}
		else
		{
			end = strstr(cur, "\r\n");

			if (end == NULL)
				/* The command is still not complete. */
				break;

			*end = '\0';
			end += 2;
			cur_len = end - cur;
		}

		servconn->rx_len -= cur_len;

		if (servconn->payload_len)
		{
			nateon_cmdproc_process_payload(servconn->cmdproc, cur, cur_len);
			servconn->payload_len = 0;
		}
		else
		{
			nateon_cmdproc_process_cmd_text(servconn->cmdproc, cur);
		}
	} while (servconn->connected && !servconn->wasted && servconn->rx_len > 0);

	if (servconn->connected && !servconn->wasted)
	{
		if (servconn->rx_len > 0)
			servconn->rx_buf = g_memdup(cur, servconn->rx_len);
		else
			servconn->rx_buf = NULL;
	}

	servconn->processing = FALSE;

	if (servconn->wasted)
		nateon_servconn_destroy(servconn);

	g_free(old_rx_buf);
}

//#if 0
//static int
//create_listener(int port)
//{
//	int fd;
//	const int on = 1;
//
//#if 0
//	struct addrinfo hints;
//	struct addrinfo *c, *res;
//	char port_str[5];
//
//	snprintf(port_str, sizeof(port_str), "%d", port);
//
//	memset(&hints, 0, sizeof(hints));
//
//	hints.ai_flags = AI_PASSIVE;
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//
//	if (getaddrinfo(NULL, port_str, &hints, &res) != 0)
//	{
//		purple_debug_error("nateon", "Could not get address info: %s.\n",
//						 port_str);
//		return -1;
//	}
//
//	for (c = res; c != NULL; c = c->ai_next)
//	{
//		fd = socket(c->ai_family, c->ai_socktype, c->ai_protocol);
//
//		if (fd < 0)
//			continue;
//
//		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
//
//		if (bind(fd, c->ai_addr, c->ai_addrlen) == 0)
//			break;
//
//		close(fd);
//	}
//
//	if (c == NULL)
//	{
//		purple_debug_error("nateon", "Could not find socket: %s.\n", port_str);
//		return -1;
//	}
//
//	freeaddrinfo(res);
//#else
//	struct sockaddr_in sockin;
//
//	fd = socket(AF_INET, SOCK_STREAM, 0);
//
//	if (fd < 0)
//		return -1;
//
//	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0)
//	{
//		close(fd);
//		return -1;
//	}
//
//	memset(&sockin, 0, sizeof(struct sockaddr_in));
//	sockin.sin_family = AF_INET;
//	sockin.sin_port = htons(port);
//
//	if (bind(fd, (struct sockaddr *)&sockin, sizeof(struct sockaddr_in)) != 0)
//	{
//		close(fd);
//		return -1;
//	}
//#endif
//
//	if (listen (fd, 4) != 0)
//	{
//		close (fd);
//		return -1;
//	}
//
//	fcntl(fd, F_SETFL, O_NONBLOCK);
//
//	return fd;
//}
//#endif
