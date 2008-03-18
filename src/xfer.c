#include "nateon.h"
#include "xfer.h"

/* Wait this many seconds before trying FR-mode transfer */
#define NATEON_P2P_TIMEOUT_SECONDS  (5)

#define NATEON_P2P_START_PORT   (5004)
#define NATEON_P2P_END_PORT	 (6004)

static void
nateon_xfer_end(PurpleXfer *xfer);
static char*
generate_p2p_cookie(NateonSession *session)
{
	NateonUser *user;
	char *cookie;
	static guint p2p_index = 1000;

	user = session->user;

	if (p2p_index > 9999) {
		p2p_index = 1000;
	}
	/* %%FIXME: how to make cookie string ? */
	cookie = g_strdup_printf("%s:%d", user->id, ++p2p_index);

	purple_debug_info("nateon", "generated p2p_cookie:%s\n", cookie);
	return cookie;
}

static void
nateon_xfer_cancel_recv(PurpleXfer *xfer)
{
	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	/* FIXME: send FILE NACK */
	
	/* free NateonXfer */
	nateon_xfer_end(xfer);
}

static void
nateon_xfer_request_denied(PurpleXfer *xfer)
{
	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	/* FIXME: free NateonXfer */
	nateon_xfer_end(xfer);
}

static void
nateon_xfer_write_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	NateonXferConnection *conn = data;
	int ret, writelen;

	purple_debug_info("nateon", "write_cb\n");
	writelen = purple_circ_buffer_get_max_read(conn->tx_buf);

	if (writelen == 0) {
		purple_input_remove(conn->tx_handler);
		conn->tx_handler = -1;
		return;
	}

	ret = write(conn->fd, conn->tx_buf->outptr, writelen);

	if (ret < 0 && errno == EAGAIN)
		return;
	else if (ret <= 0) {
		//nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_WRITE);
		purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "socket write error");
		return;
	}

	purple_circ_buffer_mark_read(conn->tx_buf, ret);
}

static ssize_t
nateon_xfer_write(NateonXferConnection *conn, const char *buf, size_t len)
{
	ssize_t ret = 0;

	g_return_val_if_fail(conn != NULL, 0);

	if (conn->tx_handler == -1) {
		ret = write(conn->fd, buf, len);
	} else {
		ret = -1;
		errno = EAGAIN;
	}

	if (ret < 0 && errno == EAGAIN)
		ret = 0;
	if (ret < len) {
		if (conn->tx_handler == -1)
			conn->tx_handler = purple_input_add(
					conn->fd, PURPLE_INPUT_WRITE,
					nateon_xfer_write_cb, conn);
		purple_circ_buffer_append(conn->tx_buf, buf + ret, len - ret);
	}

	if (ret == -1)
	{
		purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "socket write error");
		//nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_WRITE);
	}

	return ret;
}

static void
nateon_xfer_process_p2p_cmd(NateonXfer *xfer, char *cmd)
{
	gchar **split;
	gint n_split;
	gchar *buf;

	split = g_strsplit(cmd, " ", 0);
	for (n_split=0; split[n_split]; n_split++) { }

	if (!strncmp(split[0], "ATHC", 4) && strstr(cmd, "6004")) 
	{
		buf = g_strdup_printf("ATHC %d 100 6004 0\r\n", atoi(split[1]));
		nateon_xfer_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);

		buf = g_strdup_printf("FILE 0 ACCEPT %s 0\r\n", xfer->file_cookie);
		nateon_xfer_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);

	}

	else if ( (n_split == 4) && !strncmp(split[0], "ATHC", 4) && !strncmp(split[1], "0", 1) &&
			!strncmp(split[2], "100", 3) && !strncmp(split[3], "0", 1))
	{
		buf = g_strdup_printf("FILE 0 ACCEPT %s 0\r\n", xfer->file_cookie);
		nateon_xfer_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);
	}

	else if (!strncmp(split[0], "FILE", 4) && !strncmp(split[2], "INFO", 4)) 
	{
		buf = g_strdup_printf("FILE 0 START 0 0\r\n");
		nateon_xfer_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);
	}

	else if (!strncmp(split[0], "FILE", 4) && !strncmp(split[2], "DATA", 4)) 
	{
		xfer->chunk_processing = TRUE;
		xfer->chunk_len = atoi(split[3]);
		xfer->chunk_cur_len = 0;
	}

	/* FR commands */

	else if (!strncmp(split[0], "FRIN", 4) && !strncmp(split[2], "100", 3))
	{
		/* Send REQC FR packet to file sender */
		gchar *payload;
		gint payload_len;
		NateonTransaction *trans;

		if (!(xfer->p2p_cookie)) {
			xfer->p2p_cookie = generate_p2p_cookie(xfer->session);
		}
		payload = g_strdup_printf("REQC FR %s:%d %s %s\r\n", 
						xfer->fr_ip, xfer->fr_port, 
						xfer->p2p_cookie, xfer->fr_authkey);
		payload_len = strlen(payload);

		trans = nateon_transaction_new(xfer->session->notification->cmdproc, "CTOC", "%s N %d", \
				purple_xfer_get_remote_user(xfer->prpl_xfer), payload_len);
		nateon_transaction_set_payload(trans, payload, payload_len);

		g_free(payload);

		nateon_cmdproc_send_trans(xfer->session->notification->cmdproc, trans);

	}

	g_strfreev(split);
}

static gssize 
nateon_xfer_read(guchar **buffer, PurpleXfer *xfer)
{
	NateonXfer *nate_xfer;
	NateonXferConnection *conn;
	int len, cur_len;
	char buf[NATEON_BUF_LEN];
	char *end, *cur, *old_rx_buf;
	int buffer_len;

	*buffer = NULL;
	buffer_len = 0;

	nate_xfer = xfer->data;

	if (purple_xfer_get_type(xfer) == PURPLE_XFER_RECEIVE) {
		conn = &nate_xfer->conn;
	
		len = read(conn->fd, buf, sizeof(buf) - 1);

		if (len <= 0) {
			if (nate_xfer->recv_len == purple_xfer_get_size(xfer)) {
				gchar *buf;
				purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "file transfer complete");
				purple_xfer_set_completed(xfer, TRUE);

				/* Send FILE END to sender */
				buf = g_strdup_printf("ATHC 0 END N 0\r\n");
				nateon_xfer_write(conn, buf, strlen(buf));
				g_free(buf);
				return 0;
			}

			if (len < 0 && errno == EAGAIN) {
				return 0;
			} else {
				/* FIXME handle error */
				purple_debug_info("nateon", "%s:read_error", __FUNCTION__);
				//purple_debug_error("nateon", "xfer_read read error, len: %d error: %s\n", len, strerror(errno));
				return -1;
			}
		}

		buf[len] = '\0';
		conn->rx_buf = g_realloc(conn->rx_buf, len + conn->rx_len + 1);
		memcpy(conn->rx_buf + conn->rx_len, buf, len + 1);
		conn->rx_len += len;
		end = old_rx_buf = conn->rx_buf;

		do
		{
			if (nate_xfer->chunk_processing) {
				/* Receiving real file data */

				if (conn->rx_len < nate_xfer->chunk_len - nate_xfer->chunk_cur_len) {
					cur_len = conn->rx_len;
				} else {
					cur_len = nate_xfer->chunk_len - nate_xfer->chunk_cur_len;
				}

				*buffer = g_realloc(*buffer, buffer_len + cur_len);
				memcpy(*buffer + buffer_len, end, cur_len);
				buffer_len += cur_len;
				nate_xfer->recv_len += cur_len;

				end += cur_len;
				conn->rx_len -= cur_len;
				nate_xfer->chunk_cur_len += cur_len;

				if (nate_xfer->chunk_cur_len == nate_xfer->chunk_len) {
					nate_xfer->chunk_processing = FALSE;
					nate_xfer->chunk_cur_len = 0;
					nate_xfer->chunk_len = 0;
				}

				if (nate_xfer->recv_len == purple_xfer_get_size(xfer)) {
					gchar *buf;

					purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "file transfer complete");
					purple_xfer_set_completed(xfer, TRUE);

					/* Send FILE END to sender */
					buf = g_strdup_printf("ATHC 0 END N 0\r\n");
					nateon_xfer_write(&nate_xfer->conn, buf, strlen(buf));
					g_free(buf);
				}
			} else {	/* processing p2p command */
				cur = end;
				end = strstr(cur, "\r\n");
				if (end == NULL)
				{
					/* The command is still not complete. */
					break;
				}
				*end = '\0';
				end += 2;
				cur_len = end - cur;

				conn->rx_len -= cur_len;

				nateon_xfer_process_p2p_cmd(nate_xfer, cur);
			}
		} while (conn->rx_len > 0);

		if (conn->rx_len > 0)
			conn->rx_buf = g_memdup(cur, conn->rx_len);
		else
			conn->rx_buf = NULL;

		g_free(old_rx_buf);

		return buffer_len;
	}

	return 0;
}

static NateonXfer*
nateon_xfer_find_by_p2pcookie(NateonSession *session, char *cookie)
{
	GList *l;

	for (l = session->xfers; l != NULL; l = l->next)
	{
		NateonXfer *xfer;

		xfer = l->data;

		if (xfer->p2p_cookie && !strcmp(xfer->p2p_cookie, cookie)) {
			return xfer;
		}
	}
	return NULL;
}

static void
p2p_accept_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	NateonXfer *nate_xfer = data;
	int fd;

	if (nate_xfer->conntype != NATEON_XFER_CONN_NONE)
	{
		/* already connected p2p session exists. drop this connection */
		purple_debug_info("nateon", "%s:drop duplicate connection\n", __FUNCTION__);
		purple_input_remove(nate_xfer->p2p_listen_pa);
		close(source);
		nate_xfer->p2p_listen_pa = -1;
		return;
	}

	fd = accept(source, NULL, NULL);
	if (fd < 0)
	{
		/* accept failed. wait for outgoing p2p or fr-conn to succeed */
		purple_debug_info("nateon", "%s:accept error\n", __FUNCTION__);
		purple_input_remove(nate_xfer->p2p_listen_pa);
		close(source);
		nate_xfer->p2p_listen_pa = -1;
		return;
	}

	purple_debug_info("nateon", "%s:using incoming connection for p2p\n", __FUNCTION__);
	nate_xfer->conntype = NATEON_XFER_CONN_P2P;
	nate_xfer->conn.fd = fd;
	purple_xfer_start(nate_xfer->prpl_xfer, fd, NULL, 0);

	purple_input_remove(nate_xfer->p2p_listen_pa);
	close(source);
	nate_xfer->p2p_listen_pa = -1;

	/* cancel other connecting processes */
	if (nate_xfer->p2p_connect_data) {
		purple_proxy_connect_cancel(nate_xfer->p2p_connect_data);
		nate_xfer->p2p_connect_data = NULL;
	}
	if (nate_xfer->p2p_timer) {
		purple_timeout_remove(nate_xfer->p2p_timer);
		nate_xfer->p2p_timer = 0;
	}
}

static void
p2p_listen_cb(int listenfd, gpointer data)
{
	NateonXfer *nate_xfer = data;

	if (listenfd == -1) {
		/* FIXME handle error */
		/*
		purple_connection_error_reason(sip->gc,
			PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
			_("Could not create listen socket"));
		*/
		return;
	}

	if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_RECEIVE) {
		char *payload;
		size_t payload_len;
		NateonTransaction *trans;

		nate_xfer->p2p_listen_data = NULL;
		nate_xfer->p2p_listen_port = purple_network_get_port_from_fd(listenfd);
		nate_xfer->my_ip = g_strdup(purple_network_get_my_ip(listenfd));
		nate_xfer->p2p_listen_pa = purple_input_add(listenfd, PURPLE_INPUT_READ,
				p2p_accept_cb, nate_xfer);

		/* Send REQC packet to file sender */
		nate_xfer->p2p_cookie = generate_p2p_cookie(nate_xfer->session);
		payload = g_strdup_printf("REQC NEW %s:%d %s\r\n", 
						nate_xfer->my_ip, nate_xfer->p2p_listen_port, nate_xfer->p2p_cookie);
		payload_len = strlen(payload);

		trans = nateon_transaction_new(nate_xfer->session->notification->cmdproc, "CTOC", \
				"%s N %d", purple_xfer_get_remote_user(nate_xfer->prpl_xfer), payload_len);

		nateon_transaction_set_payload(trans, payload, payload_len);

		g_free(payload);

		nateon_cmdproc_send_trans(nate_xfer->session->notification->cmdproc, trans);
	}
}

static gboolean
p2p_timeout_cb(gpointer data)
{
	NateonXfer *xfer = data;

	if (xfer->conntype == NATEON_XFER_CONN_NONE)
	{
		NateonTransaction *trans;
		purple_debug_info("nateon", "p2p connection timed out. Connecting to FR server\n");

		/* Send REFR to initiate FR-mode xfer */
		trans = nateon_transaction_new(xfer->session->notification->cmdproc, "REFR", "%s", \
									xfer->who);
		nateon_cmdproc_send_trans(xfer->session->notification->cmdproc, trans);
		xfer->fr_initiate_trid = trans->trId;
	}

	return FALSE;
}

static void
nateon_xfer_init(PurpleXfer *xfer)
{
	NateonXfer *nate_xfer;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	nate_xfer = xfer->data;

	nate_xfer->conntype = NATEON_XFER_CONN_NONE;

	if (purple_xfer_get_type(xfer) == PURPLE_XFER_RECEIVE)
	{
		nate_xfer->p2p_listen_data = purple_network_listen_range(NATEON_P2P_START_PORT,\
				NATEON_P2P_END_PORT, SOCK_STREAM, p2p_listen_cb, nate_xfer);

		/* Start timer for FR-mode connection */
		purple_debug_info("nateon", "starting timer for fr_connect\n");
		nate_xfer->p2p_timer = purple_timeout_add(NATEON_P2P_TIMEOUT_SECONDS*1000, \
				p2p_timeout_cb, nate_xfer);
	}
}

static void
nateon_xfer_end(PurpleXfer *xfer)
{
	NateonXfer *nate_xfer;
	NateonSession *session;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__); 

	nate_xfer = xfer->data;

	if (nate_xfer) {
		session = nate_xfer->session;
		nate_xfer->session = NULL;
		if (nate_xfer->who)
			g_free(nate_xfer->who);
		if (nate_xfer->my_ip)
			g_free(nate_xfer->my_ip);
		if (nate_xfer->file_cookie)
			g_free(nate_xfer->file_cookie);

		if (nate_xfer->p2p_cookie)
			g_free(nate_xfer->p2p_cookie);
		if (nate_xfer->p2p_listen_data)
			purple_network_listen_cancel(nate_xfer->p2p_listen_data);
		if (nate_xfer->p2p_connect_data)
			purple_proxy_connect_cancel(nate_xfer->p2p_connect_data);
		if (nate_xfer->p2p_listen_pa > 0)
			purple_input_remove(nate_xfer->p2p_listen_pa);
		if (nate_xfer->p2p_timer)
			purple_timeout_remove(nate_xfer->p2p_timer);

		if (nate_xfer->fr_ip)
			g_free(nate_xfer->fr_ip);
		if (nate_xfer->fr_authkey)
			g_free(nate_xfer->fr_authkey);
		if (nate_xfer->fr_connect_data)
			purple_proxy_connect_cancel(nate_xfer->fr_connect_data);

		if (nate_xfer->conn.rx_buf)
			g_free(nate_xfer->conn.rx_buf);
		if (nate_xfer->conn.tx_buf)
			purple_circ_buffer_destroy(nate_xfer->conn.tx_buf);
		if (nate_xfer->conn.tx_handler > 0)
			purple_input_remove(nate_xfer->conn.tx_handler);

		session->xfers = g_list_remove(session->xfers, nate_xfer);
		g_free(nate_xfer);
	}
	xfer->data = NULL;
	return;
}

static NateonXfer*
nateon_xfer_new(NateonSession *session, PurpleXferType type, const char *who)
{
	NateonXfer *xfer;

	xfer = g_new0(NateonXfer, 1);

	xfer->session = session;

	xfer->prpl_xfer = purple_xfer_new(session->account, type, who);

	xfer->prpl_xfer->data = xfer;

	xfer->who = g_strdup(who);

	session->xfers = g_list_append(session->xfers, xfer);

	xfer->conn.tx_handler = -1;
	xfer->p2p_listen_pa = -1;

	return xfer;
}

static void
p2p_connect_cb(gpointer data, gint source, const char *error_message)
{
	NateonXfer *nate_xfer = data;

	gchar *buf;

	nate_xfer->p2p_connect_data = NULL;

	if (source < 0)
	{
		/* Simply wait for FR to begin */
		purple_debug_info("nateon", "%s:Connect failed. err: [%s]\n", __FUNCTION__, error_message);
		return;
	}

	if (nate_xfer->conntype != NATEON_XFER_CONN_NONE)
	{
		/* already connected session exists. drop this connection */
		purple_debug_info("nateon", "%s:drop duplicate connection\n", __FUNCTION__);
		close(source);
		return;
	}

	purple_debug_info("nateon", "%s:using outgoing connection for p2p\n", __FUNCTION__);
	nate_xfer->conntype = NATEON_XFER_CONN_P2P;
	nate_xfer->conn.fd = source;

	buf = g_strdup_printf("ATHC 0 %s %s %s 6004 0\r\n",
			nate_xfer->session->user->account_name,
			nate_xfer->who,
			nate_xfer->p2p_cookie);
	nateon_xfer_write(&nate_xfer->conn, buf, strlen(buf));
	g_free(buf);

	purple_xfer_start(nate_xfer->prpl_xfer, source, NULL, 0);

	/* cancel other connecting processes */
	if (nate_xfer->p2p_listen_data) {
		purple_network_listen_cancel(nate_xfer->p2p_listen_data);
		nate_xfer->p2p_listen_data = NULL;
	}
	if (nate_xfer->p2p_timer) {
		purple_timeout_remove(nate_xfer->p2p_timer);
		nate_xfer->p2p_timer = 0;
	}
	return;
}

static void
fr_connect_cb(gpointer data, gint source, const char *error_message)
{
	NateonXfer *nate_xfer = data;
	gchar *buf;

	nate_xfer->fr_connect_data = NULL;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	if (source < 0)
	{
		/* FIXME: Handle error. This was our last try.. */
		purple_debug_info("nateon", "%s:Connect failed. err: [%s]\n", __FUNCTION__, error_message);
		return;
	}

	if (nate_xfer->conntype != NATEON_XFER_CONN_NONE)
	{
		/* already connected p2p session exists. drop this connection */
		purple_debug_info("nateon", "%s:drop duplicate connection\n", __FUNCTION__);
		close(source);
		return;
	}

	nate_xfer->conntype = NATEON_XFER_CONN_FR;

	/* cancel p2p connection tries */
	if (nate_xfer->p2p_connect_data)
	{
		purple_proxy_connect_cancel(nate_xfer->p2p_connect_data);
		nate_xfer->p2p_connect_data = NULL;
	}
	if (nate_xfer->p2p_listen_data)
	{
		purple_network_listen_cancel(nate_xfer->p2p_listen_data);
		nate_xfer->p2p_listen_data = NULL;
	}
	if (nate_xfer->p2p_listen_pa > 0)
	{
		purple_input_remove(nate_xfer->p2p_listen_pa);
		nate_xfer->p2p_listen_pa = -1;
	}

	purple_debug_info("nateon", "%s:using fr connection for file transfer\n", __FUNCTION__);
	nate_xfer->conn.fd = source;

	buf = g_strdup_printf("FRIN 0 %s %s\r\n", nate_xfer->session->user->account_name, 
										nate_xfer->fr_authkey);
	nateon_xfer_write(&nate_xfer->conn, buf, strlen(buf));
	g_free(buf);

	purple_xfer_start(nate_xfer->prpl_xfer, source, NULL, 0);
}

void
nateon_xfer_parse_refr(NateonSession *session, char **params, int param_count)
{
	NateonXfer *xfer = NULL;
	GList *l;

	if (param_count == 4) {
		/* find xfer by fr trid */
		xfer = NULL;
		for (l = session->xfers; l != NULL; l = l->next)
		{
			NateonXfer *t_xfer;
			t_xfer = l->data;
			if (t_xfer->fr_initiate_trid == atoi(params[0])) {
				xfer = t_xfer;
				break;
			}
		}
		if (!xfer || xfer->conntype != NATEON_XFER_CONN_NONE)
		{
			return;
		}

		xfer->fr_ip = g_strdup(params[1]);
		xfer->fr_port = atoi(params[2]);
		xfer->fr_authkey = g_strdup(params[3]);

		xfer->fr_connect_data = purple_proxy_connect(NULL, session->account, xfer->fr_ip, xfer->fr_port,
				fr_connect_cb, xfer);
	}
}

void
nateon_xfer_parse_reqc(NateonSession *session, char **params, int param_count)
{
	if (param_count >= 3 && !strncmp(params[0], "RES", 3)) {
		NateonXfer *xfer;
		char **split;

		xfer = nateon_xfer_find_by_p2pcookie(session, params[2]);
		if (!xfer) {
			purple_debug_info("nateon", "no matching p2pcookie found for this xfer\n");
			return;
		}

		split = g_strsplit(params[1], ":", 2);
		xfer->p2p_connect_data = purple_proxy_connect(NULL, session->account, split[0], atoi(split[1]),
				p2p_connect_cb, xfer);

	}
}

void
nateon_xfer_receive_file(NateonSession *session, NateonSwitchBoard *swboard, \
		const char *who, const char *filename, const int filesize, const char *cookie)
{
	NateonXfer *xfer;

	xfer = nateon_xfer_new(session, PURPLE_XFER_RECEIVE, who);

	xfer->file_cookie = g_strdup(cookie);
	xfer->swboard = swboard;
	purple_xfer_set_filename(xfer->prpl_xfer, filename);
	purple_xfer_set_size(xfer->prpl_xfer, filesize);

	purple_xfer_set_init_fnc(xfer->prpl_xfer, nateon_xfer_init);
	purple_xfer_set_cancel_recv_fnc(xfer->prpl_xfer, nateon_xfer_cancel_recv);
	purple_xfer_set_request_denied_fnc(xfer->prpl_xfer, nateon_xfer_request_denied);
	purple_xfer_set_read_fnc(xfer->prpl_xfer, nateon_xfer_read);
	purple_xfer_set_end_fnc(xfer->prpl_xfer, nateon_xfer_end);

	purple_xfer_request(xfer->prpl_xfer);
}
