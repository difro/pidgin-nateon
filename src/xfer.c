#include "nateon.h"
#include "xfer.h"

/* Wait this many seconds before trying FR-mode transfer */
#define NATEON_P2P_TIMEOUT_SECONDS  (5)

#define NATEON_P2P_START_PORT   (5004)
#define NATEON_P2P_END_PORT	 (6004)

static void nateon_xfer_end(PurpleXfer *xfer);
static ssize_t nateon_xfer_sock_write(NateonXferConnection *conn, const char *buf, size_t len);
static void nateon_xfer_send_next(NateonXfer *nate_xfer);
gint xfer_sig_cmp( gconstpointer a, gconstpointer b );

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
	cookie = g_strdup_printf("%s:%d", user->id, ++p2p_index);

	purple_debug_info("nateon", "generated p2p_cookie:%s\n", cookie);
	return cookie;
}

static void
nateon_xfer_cancel_recv_dummy(PurpleXfer *xfer)
{
	return;
}

static void
nateon_xfer_cancel_recv(PurpleXfer *xfer)
{
	NateonXfer *nate_xfer = xfer->data;

	char *filename;

	NateonTransaction *trans;
	NateonCmdProc *cmdproc = nate_xfer->swboard->cmdproc;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	filename = purple_strreplace(xfer->filename, " ", "%20");

	// send
	// WHSP 0 ccc@nate.com|dpc_10605:15962|NDhA FILE CANCEL%091%09723:10009502162:75
	// ccc is the sender name.
	trans = nateon_transaction_new(
		cmdproc, "WHSP", "%s FILE CANCEL%%091%%09%s|%d|%s",
			nate_xfer->who, filename,
			xfer->size, nate_xfer->file_cookie );

	nateon_cmdproc_send_trans(cmdproc, trans);

	g_free(filename);

	/* free NateonXfer */
	nateon_xfer_end(xfer);
}

static void
nateon_xfer_cancel_recv_old(PurpleXfer *xfer)
{
	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	/* FIXME: send FILE NACK */
	
	/* free NateonXfer */
	nateon_xfer_end(xfer);
}

static void
nateon_xfer_cancel_send(PurpleXfer *xfer)
{
	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	nateon_xfer_cancel_recv( xfer );
}

static void
nateon_xfer_deny_request(PurpleXfer *xfer)
{
	NateonXfer *nate_xfer = xfer->data;
	char *filename;

	NateonTransaction *trans;
	NateonCmdProc *cmdproc = nate_xfer->swboard->cmdproc;

	purple_debug_info("nateon", "%s\n", __FUNCTION__);

	filename = purple_strreplace(xfer->filename, " ", "%20");

	// send this
	// WHSP 0 xxx@nate.com|dpc_04903:8994|NDhA FILE NACK%091%09yr|2714|266:10009502162:427
	trans = nateon_transaction_new(
		cmdproc, "WHSP", "%s FILE NACK%%091%%09%s|%d|%s",
			nate_xfer->who, filename,
			xfer->size, nate_xfer->file_cookie );

	nateon_cmdproc_send_trans(cmdproc, trans);

	g_free(filename);

	/* free NateonXfer */
	nateon_xfer_end(xfer);
}

static void
nateon_xfer_process_p2p_cmd_receive(NateonXfer *xfer, char *cmd)
{
	gchar **split;
	gint n_split;
	gchar *buf;

	split = g_strsplit(cmd, " ", 0);
	for (n_split=0; split[n_split]; n_split++) { }

	if (!strncmp(split[0], "ATHC", 4) && strstr(cmd, "6004")) 
	{
		buf = g_strdup_printf("ATHC %d 100 6004 0\r\n", atoi(split[1]));
		nateon_xfer_sock_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);

		buf = g_strdup_printf("FILE 0 ACCEPT %s 0\r\n", xfer->file_cookie);
		nateon_xfer_sock_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);

	}

	else if ( (n_split == 4) && !strncmp(split[0], "ATHC", 4) && !strncmp(split[1], "0", 1) &&
			!strncmp(split[2], "100", 3) && !strncmp(split[3], "0", 1))
	{
		buf = g_strdup_printf("FILE 0 ACCEPT %s 0\r\n", xfer->file_cookie);
		nateon_xfer_sock_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);
	}

	else if (!strncmp(split[0], "FILE", 4) && !strncmp(split[2], "INFO", 4)) 
	{
		buf = g_strdup_printf("FILE 0 START 0 0\r\n");
		nateon_xfer_sock_write(&xfer->conn, buf, strlen(buf));
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

static void
nateon_xfer_process_p2p_cmd_send(NateonXfer *xfer, char *cmd)
{
	gchar **split;
	gint n_split;
	gchar *buf;

	split = g_strsplit(cmd, " ", 0);
	for (n_split=0; split[n_split]; n_split++) { }

	if (!strncmp(split[0], "ATHC", 4) && strstr(cmd, "6004")) 
	{
		buf = g_strdup_printf("ATHC %d 100 6004 0\r\n", atoi(split[1]));
		nateon_xfer_sock_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);
	}

	else if ( (n_split == 4) && !strncmp(split[0], "ATHC", 4) && !strncmp(split[1], "0", 1) &&
			!strncmp(split[2], "100", 3) && !strncmp(split[3], "0", 1))
	{
		/* do nothing */
	}

	else if (!strncmp(split[0], "FILE", 4) && !strncmp(split[2], "ACCEPT", 6))
	{
		if (strcmp(xfer->file_cookie, split[3]) != 0)
		{
			purple_debug_info("nateon", "%s:file cookie does not match!\n", __FUNCTION__);
			purple_xfer_cancel_remote(xfer->prpl_xfer);
			return;
		}
        buf = g_strdup_printf("FILE %s INFO FILENAME %ld CHAT 0\r\n",
				split[1], purple_xfer_get_size(xfer->prpl_xfer));
		nateon_xfer_sock_write(&xfer->conn, buf, strlen(buf));
		g_free(buf);
	}

	else if (!strncmp(split[0], "FILE", 4) && !strncmp(split[2], "START", 5)) 
	{
		/* Start sending data */
		xfer->send_data_trid = atoi(split[1]);
		nateon_xfer_send_next(xfer);
	}

	else if (!strncmp(split[0], "FILE", 4) && !strncmp(split[2], "END", 3)) 
	{
        purple_debug_info("nateon", "%s: recved FILE END. file transfer complete\n", __FUNCTION__);
		purple_xfer_set_completed(xfer->prpl_xfer, TRUE);
	}

	g_strfreev(split);
}

static gssize 
nateon_xfer_sock_read(NateonXferConnection *conn, guchar **buffer)
{
	NateonXfer *nate_xfer;
	int len, cur_len;
	char buf[NATEON_BUF_LEN];
	char *end, *cur, *old_rx_buf;
	int buffer_len;

	*buffer = NULL;
	buffer_len = 0;
	nate_xfer = conn->nate_xfer;

	if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_RECEIVE) {
		conn = &nate_xfer->conn;
	
		len = read(conn->fd, buf, sizeof(buf) - 1);

		if (len <= 0) {
			if (len < 0 && errno == EAGAIN) {
				return 0;
			} else {
				if (nate_xfer->recv_len == purple_xfer_get_size(nate_xfer->prpl_xfer))
				{
					purple_xfer_set_completed(nate_xfer->prpl_xfer, TRUE);
					return 0;
				}
				/* FIXME handle error */
				purple_debug_info("nateon", "%s:read_error\n", __FUNCTION__);
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

				// Transfer complete!
				if (nate_xfer->recv_len == purple_xfer_get_size(nate_xfer->prpl_xfer)) {
					gchar *buf;

					purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "file transfer complete");
					purple_xfer_set_completed(nate_xfer->prpl_xfer, TRUE);

					/* Send FILE END to sender */
					buf = g_strdup_printf("FILE 2 END N 0\r\n");
					nateon_xfer_sock_write(&nate_xfer->conn, buf, strlen(buf));
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

				nateon_xfer_process_p2p_cmd_receive(nate_xfer, cur);
			}
		} while (conn->rx_len > 0);

		if (conn->rx_len > 0)
			conn->rx_buf = g_memdup(cur, conn->rx_len);
		else
			conn->rx_buf = NULL;

		g_free(old_rx_buf);

		return buffer_len;
	}

	else if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_SEND)
	{
		conn = &nate_xfer->conn;
	
		len = read(conn->fd, buf, sizeof(buf) - 1);

		if (len <= 0) {
			if (len < 0 && errno == EAGAIN) {
				return 0;
			} else {
				/* FIXME handle error */
				purple_debug_info("nateon", "%s:read_error\n", __FUNCTION__);
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
			/* processing p2p command */
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

			nateon_xfer_process_p2p_cmd_send(nate_xfer, cur);
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

static void
nateon_xfer_sock_read_cb(gpointer data, gint source, PurpleInputCondition condition)
{
	NateonXfer *nate_xfer = data;
	gssize r = 0;
	guchar *buffer = NULL;

	if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_RECEIVE)
	{
		r = nateon_xfer_sock_read(&nate_xfer->conn, &buffer);
		printf( "JDJ: RECEIVED %d BYTES\n", r );
		if (r > 0)
		{
			const size_t wc = fwrite(buffer, 1, r, nate_xfer->dest_fp);
			if (wc != r)
			{
				purple_debug_error("nateon", "%s:Unable to write whole buffer.\n", __FUNCTION__);
				if (nate_xfer->content_type == NATEON_XFER_CONTENT_FILE)
				{
					purple_xfer_cancel_remote(nate_xfer->prpl_xfer);
				}
				g_free(buffer);
				return;
			}

			// Let libpurple know the progress.
			if (nate_xfer->content_type == NATEON_XFER_CONTENT_FILE)
			{
				purple_xfer_set_bytes_sent(nate_xfer->prpl_xfer, nate_xfer->recv_len); // actually, received bytes.
				purple_xfer_update_progress(nate_xfer->prpl_xfer);
			}
		}
		else if (r < 0)
		{
			if (nate_xfer->content_type == NATEON_XFER_CONTENT_FILE)
			{
				purple_xfer_cancel_remote(nate_xfer->prpl_xfer);
			}
			else if (nate_xfer->content_type == NATEON_XFER_CONTENT_BUDDYIMG)
			{
				purple_debug_info("nateon", "%s:Error fetching buddy icon\n",
						__FUNCTION__);
				nateon_xfer_cancel_recv(nate_xfer->prpl_xfer);
				return;
			}

		}
	}
	else if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_SEND)
	{
		r = nateon_xfer_sock_read(&nate_xfer->conn, &buffer);
		if (r < 0)
		{
			if (purple_xfer_get_bytes_sent(nate_xfer->prpl_xfer) == \
					purple_xfer_get_size(nate_xfer->prpl_xfer) )
			{
				purple_xfer_set_completed(nate_xfer->prpl_xfer, TRUE);
			}
		}
	}

	if (r > 0)
	{
		g_free(buffer);
	}

	if (purple_xfer_is_completed(nate_xfer->prpl_xfer))
	{
		if (nate_xfer->dest_fp)
		{
			fclose(nate_xfer->dest_fp);
			nate_xfer->dest_fp = NULL;
		}
		if (nate_xfer->content_type == NATEON_XFER_CONTENT_BUDDYIMG)
		{
			const gchar *filepath;
			guchar *imgbuf;
			gint filesize;
			gint ret;
			FILE *fp;

			filesize = purple_xfer_get_size(nate_xfer->prpl_xfer);
			imgbuf = g_malloc(filesize);
			filepath = purple_xfer_get_local_filename(nate_xfer->prpl_xfer);
            fp = g_fopen(filepath, "rb");
			if (fp)
			{
				ret = fread(imgbuf, 1, filesize, fp);
				fclose(fp);
				if (ret == filesize)
				{
					purple_buddy_icons_set_for_user(nate_xfer->session->account,
							nate_xfer->who, imgbuf, filesize, 
							purple_xfer_get_filename(nate_xfer->prpl_xfer));
				}
			}
			g_unlink(filepath);
		}
		purple_xfer_end(nate_xfer->prpl_xfer);
	}

	return;
}

static void
nateon_xfer_sock_write_cb(gpointer data, gint source, PurpleInputCondition cond)
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
		purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "socket write error");
		return;
	}

	purple_circ_buffer_mark_read(conn->tx_buf, ret);
}

static ssize_t
nateon_xfer_sock_write(NateonXferConnection *conn, const char *buf, size_t len)
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
					nateon_xfer_sock_write_cb, conn);
		purple_circ_buffer_append(conn->tx_buf, buf + ret, len - ret);
	}

	if (ret == -1)
	{
		purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "socket write error");
	}

	return ret;
}

static ssize_t nateon_xfer_send_data(NateonXfer *nate_xfer, const char *buf, size_t len);

static void
nateon_xfer_send_next(NateonXfer *nate_xfer)
{
	NateonXferConnection *conn;
	gchar *buf;
	int cmd_len;

	conn = &nate_xfer->conn;

	if (nate_xfer->sent_len == purple_xfer_get_size(nate_xfer->prpl_xfer))
	{
		purple_input_remove(conn->tx_handler);
		conn->tx_handler = -1;
		return;
	}

	// determine chunk length.
	if (purple_xfer_get_size(nate_xfer->prpl_xfer)-nate_xfer->sent_len > NATEON_XFER_SEND_BUFFER_SIZE)
	{
		nate_xfer->chunk_len = NATEON_XFER_SEND_BUFFER_SIZE;
	}
	else
	{
		nate_xfer->chunk_len = purple_xfer_get_size(nate_xfer->prpl_xfer) - nate_xfer->sent_len;
	}

	buf = g_strdup_printf("FILE %d DATA %d\r\n",
			nate_xfer->send_data_trid, nate_xfer->chunk_len);
	cmd_len = strlen(buf);
	buf = g_realloc(buf, cmd_len + nate_xfer->chunk_len);
	fread(buf + cmd_len, 1, nate_xfer->chunk_len, nate_xfer->local_fp);
	nateon_xfer_send_data(nate_xfer, buf, cmd_len + nate_xfer->chunk_len);
	g_free(buf);
	nate_xfer->send_data_trid++;
}

static void
nateon_xfer_send_data_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	NateonXfer *nate_xfer = data;
	NateonXferConnection *conn = &nate_xfer->conn;
	int ret, writelen;

	writelen = purple_circ_buffer_get_max_read(conn->tx_buf);

	if (writelen == 0)
	{
		purple_input_remove(conn->tx_handler);
		conn->tx_handler = -1;
		nate_xfer->sent_len += nate_xfer->chunk_len;
		nateon_xfer_send_next(nate_xfer);
		purple_xfer_set_bytes_sent(nate_xfer->prpl_xfer, nate_xfer->sent_len);
		purple_xfer_update_progress(nate_xfer->prpl_xfer);
		return;
	}

	ret = write(conn->fd, conn->tx_buf->outptr, writelen);

	if (ret < 0 && errno == EAGAIN)
		return;
	else if (ret <= 0) {
		//nateon_servconn_got_error(servconn, NATEON_SERVCONN_ERROR_WRITE);
		purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "socket write error");
		purple_input_remove(conn->tx_handler);
		conn->tx_handler = -1;
		purple_xfer_cancel_remote(nate_xfer->prpl_xfer);
		return;
	}

	purple_circ_buffer_mark_read(conn->tx_buf, ret);
}

static ssize_t
nateon_xfer_send_data(NateonXfer *nate_xfer, const char *buf, size_t len)
{
	ssize_t ret = 0;
	NateonXferConnection *conn;

	conn = &nate_xfer->conn;
	if (conn->tx_handler == -1)
	{
		ret = write(conn->fd, buf, len);
	}
	else
	{
		ret = -1;
		errno = EAGAIN;
	}

	if (ret < 0 && errno == EAGAIN)
		ret = 0;
	if (ret < len)
	{
		if (conn->tx_handler == -1)
		{
			conn->tx_handler = purple_input_add( conn->fd, PURPLE_INPUT_WRITE,
					nateon_xfer_send_data_cb, nate_xfer);
		}
		purple_circ_buffer_append(conn->tx_buf, buf + ret, len - ret);
	} else if (ret == len)
	{
		nate_xfer->sent_len += nate_xfer->chunk_len;
		purple_xfer_set_bytes_sent(nate_xfer->prpl_xfer, nate_xfer->sent_len);
		purple_xfer_update_progress(nate_xfer->prpl_xfer);
		nateon_xfer_send_next(nate_xfer);
		return ret;
	}

	if (ret == -1)
	{
		purple_debug_info("nateon", "%s:%s\n", __FUNCTION__, "socket write error");
		purple_xfer_cancel_remote(nate_xfer->prpl_xfer);
	}

	return ret;

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

	purple_input_remove(nate_xfer->p2p_listen_pa);
	nate_xfer->p2p_listen_pa = -1;

	if (nate_xfer->conntype != NATEON_XFER_CONN_NONE)
	{
		/* already connected p2p session exists. drop this connection */
		purple_debug_info("nateon", "%s:drop duplicate connection\n", __FUNCTION__);
		close(source);
		return;
	}

	fd = accept(source, NULL, NULL);
	if (fd < 0)
	{
		/* accept failed. wait for outgoing p2p or fr-conn to succeed */
		purple_debug_info("nateon", "%s:accept error\n", __FUNCTION__);
		close(source);
		return;
	}

	purple_debug_info("nateon", "%s:using incoming connection for p2p\n", __FUNCTION__);
	nate_xfer->conntype = NATEON_XFER_CONN_P2P;
	nate_xfer->conn.fd = fd;

	close(source);

	purple_xfer_start(nate_xfer->prpl_xfer, -1, NULL, 0);
	nate_xfer->conn.rx_pa = purple_input_add(fd, PURPLE_INPUT_READ, \
			nateon_xfer_sock_read_cb, nate_xfer);

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

	nate_xfer->p2p_listen_data = NULL;

	if (listenfd == -1) {
		purple_debug_info("nateon", 
				"%s:Could not create listen socket. Wait for other connections.\n",
				__FUNCTION__);
		return;
	}

	nate_xfer->p2p_listen_port = purple_network_get_port_from_fd(listenfd);
	nate_xfer->my_ip = g_strdup(purple_network_get_my_ip(listenfd));
	nate_xfer->p2p_listen_fd = listenfd;
	nate_xfer->p2p_listen_pa = purple_input_add(listenfd, PURPLE_INPUT_READ,
			p2p_accept_cb, nate_xfer);

	if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_RECEIVE)
	{
		char *payload;
		size_t payload_len;
		NateonTransaction *trans;

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
	else
	{
		char *payload;
		size_t payload_len;
		NateonTransaction *trans;

		/* Send REQC packet to file sender */
		payload = g_strdup_printf("REQC RES %s:%d %s\r\n", 
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

	/* register watcher for read callback */
	purple_xfer_start(nate_xfer->prpl_xfer, -1, NULL, 0);
	nate_xfer->conn.rx_pa = purple_input_add(nate_xfer->conn.fd, PURPLE_INPUT_READ, \
			nateon_xfer_sock_read_cb, nate_xfer);

	buf = g_strdup_printf("ATHC 0 %s %s %s 6004 0\r\n",
			nate_xfer->session->user->account_name,
			nate_xfer->who,
			nate_xfer->p2p_cookie);
	nateon_xfer_sock_write(&nate_xfer->conn, buf, strlen(buf));
	g_free(buf);

	/* cancel other connecting processes */
	if (nate_xfer->p2p_listen_data)
	{
		purple_network_listen_cancel(nate_xfer->p2p_listen_data);
		nate_xfer->p2p_listen_data = NULL;
	}
	if (nate_xfer->p2p_listen_pa)
	{
		purple_input_remove(nate_xfer->p2p_listen_pa);
		nate_xfer->p2p_listen_pa = -1;
	}
	if (nate_xfer->p2p_listen_fd)
	{
		close(nate_xfer->p2p_listen_fd);
		nate_xfer->p2p_listen_fd = 0;
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
	if (nate_xfer->p2p_listen_fd)
	{
		close(nate_xfer->p2p_listen_fd);
		nate_xfer->p2p_listen_fd = 0;
	}

	purple_debug_info("nateon", "%s:using fr connection for file transfer\n", __FUNCTION__);
	nate_xfer->conn.fd = source;

	/* register watcher for read callback */
	nate_xfer->conn.rx_pa = purple_input_add(nate_xfer->conn.fd, PURPLE_INPUT_READ, \
			nateon_xfer_sock_read_cb, nate_xfer);
	purple_xfer_start(nate_xfer->prpl_xfer, -1, NULL, 0);

	if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_RECEIVE)
	{
		buf = g_strdup_printf("FRIN 0 %s %s\r\n", nate_xfer->session->user->account_name, 
				nate_xfer->fr_authkey);
		nateon_xfer_sock_write(&nate_xfer->conn, buf, strlen(buf));
		g_free(buf);
	}
	else if (purple_xfer_get_type(nate_xfer->prpl_xfer) == PURPLE_XFER_SEND)
	{
		buf = g_strdup_printf("ATHC 0 %s %s %s %s 6004 0\r\n", 
				nate_xfer->session->user->account_name, 
				nate_xfer->who,
				nate_xfer->p2p_cookie, nate_xfer->fr_authkey);
		nateon_xfer_sock_write(&nate_xfer->conn, buf, strlen(buf));
		g_free(buf);
	}
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
		if (nate_xfer->content_type == NATEON_XFER_CONTENT_FILE)
		{
            nate_xfer->dest_fp = g_fopen(purple_xfer_get_local_filename(xfer), "wb");
		}
		else if (nate_xfer->content_type == NATEON_XFER_CONTENT_BUDDYIMG)
		{
			char *filepath;
			nate_xfer->dest_fp = purple_mkstemp(&filepath, TRUE);
			purple_xfer_set_local_filename(xfer, filepath);
			g_free(filepath);
		}

		if (nate_xfer->dest_fp == NULL) {
			purple_debug_info("nateon", "%s: Error Writing File %s\n", __FUNCTION__,
					purple_xfer_get_local_filename(xfer));
			purple_xfer_cancel_local(xfer);
			return;
		}

		nate_xfer->p2p_listen_data = purple_network_listen_range(NATEON_P2P_START_PORT,\
				NATEON_P2P_END_PORT, SOCK_STREAM, p2p_listen_cb, nate_xfer);

		/* Start timer for FR-mode connection */
		purple_debug_info("nateon", "starting timer for fr_connect\n");
		nate_xfer->p2p_timer = purple_timeout_add(NATEON_P2P_TIMEOUT_SECONDS*1000, \
				p2p_timeout_cb, nate_xfer);
	}
	else if (purple_xfer_get_type(xfer) == PURPLE_XFER_SEND)
	{
		NateonSwitchBoard *swboard;
		NateonTransaction *trans;
		gchar *new_filename;

		swboard = nate_xfer->swboard;

        nate_xfer->local_fp = g_fopen(purple_xfer_get_local_filename(nate_xfer->prpl_xfer),
				"rb");
		if (!(nate_xfer->local_fp))
		{
			purple_debug_error("nateon", "%s: can't open file\n", __FUNCTION__);
			return;
		}
		fseek(nate_xfer->local_fp, 0, SEEK_SET);

		nate_xfer->file_cookie = g_strdup_printf("%d:%s:%d",
				g_random_int_range(100,999), nate_xfer->session->user->id,
				g_random_int_range(100,999));

		new_filename = purple_strreplace(purple_xfer_get_filename(nate_xfer->prpl_xfer),
											" ", "%20");

		trans = nateon_transaction_new(swboard->cmdproc, "WHSP", \
				"%s FILE REQUEST%%09%d%%09%s|%d|%s",
				nate_xfer->who, 1, new_filename,
				purple_xfer_get_size(nate_xfer->prpl_xfer),
				nate_xfer->file_cookie);
		g_free(new_filename);

		if (swboard->ready)
		{
			purple_debug_info("nateon", "[%s] send_trans\n", __FUNCTION__);
			nateon_cmdproc_send_trans(swboard->cmdproc, trans);
		}
		else
		{
			purple_debug_info("nateon", "[%s] queue_tx\n", __FUNCTION__);
			nateon_cmdproc_queue_tx(swboard->cmdproc, trans);
		}
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

		if (nate_xfer->conn.rx_pa > 0)
			purple_input_remove(nate_xfer->conn.rx_pa);
		if (nate_xfer->conn.rx_buf)
			g_free(nate_xfer->conn.rx_buf);
		if (nate_xfer->conn.tx_buf)
			purple_circ_buffer_destroy(nate_xfer->conn.tx_buf);
		if (nate_xfer->conn.tx_handler > 0)
			purple_input_remove(nate_xfer->conn.tx_handler);

		if (nate_xfer->conn.fd)
			close(nate_xfer->conn.fd);
		if (nate_xfer->local_fp)
			fclose(nate_xfer->local_fp);

		session->xfers = g_list_remove(session->xfers, nate_xfer);
		g_free(nate_xfer);
	}
	xfer->data = NULL;
	return;
}

static NateonXfer*
nateon_xfer_new(NateonSession *session, PurpleXferType type, const char *who)
{
	const char *sender_id, *dpkey;
	gchar **split;
	NateonXfer *xfer;

	// who might have dpkey appended. Separate it.
	split = g_strsplit( who, "|", 2 );
	sender_id = split[0];
	dpkey = split[1];

	xfer = g_new0(NateonXfer, 1);

	xfer->session = session;

	xfer->prpl_xfer = purple_xfer_new(session->account, type, sender_id);

	xfer->prpl_xfer->data = xfer;

	xfer->who = g_strdup(sender_id);
	xfer->dpkey = g_strdup(dpkey);

	session->xfers = g_list_append(session->xfers, xfer);

	xfer->conn.tx_handler = -1;
	xfer->conn.tx_buf = purple_circ_buffer_new(0);
	xfer->conn.nate_xfer = xfer;
	xfer->p2p_listen_pa = -1;

	g_strfreev(split);

	return xfer;
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

		xfer->fr_connect_data = purple_proxy_connect(NULL, session->account, 
				xfer->fr_ip, xfer->fr_port, fr_connect_cb, xfer);
	}
}

void
nateon_xfer_parse_reqc(NateonSession *session, NateonCmdProc *cmdproc, char **params, int param_count)
{
	NateonXfer *xfer = NULL;

	if (param_count >= 3 && !strncmp(params[0], "RES", 3)) {
		char **split;

		xfer = nateon_xfer_find_by_p2pcookie(session, params[2]);
		if (!xfer) {
			purple_debug_info("nateon", "no matching p2pcookie found for this xfer\n");
			return;
		}
		//if (xfer->conntype == NATEON_XFER_CONN_NONE)
		//{
			split = g_strsplit(params[1], ":", 2);
			xfer->p2p_connect_data = purple_proxy_connect(NULL, session->account, split[0], 
					atoi(split[1]), p2p_connect_cb, xfer);
			g_strfreev(split);
		//}
	}
	else if (param_count >= 3 && !strncmp(params[0], "NEW", 3))
	{
		char **split;
		char *peer_name;
		GList *l;
		NateonTransaction *trans;

		if (cmdproc->last_cmd && !strcmp(cmdproc->last_cmd->command, "CTOC"))
		{
			peer_name = cmdproc->last_cmd->params[1];
		}
		else
		{
			return;
		}

		for (l = session->xfers; l; l = l->next)
		{
			NateonXfer *tmp_xfer;
			tmp_xfer = l->data;
			if (purple_xfer_get_type(tmp_xfer->prpl_xfer) == PURPLE_XFER_SEND &&
					!strcmp(tmp_xfer->who, peer_name) )
			{
				xfer = tmp_xfer;
				break;
			}
		}
		if (!xfer) {
			purple_debug_info("nateon", "no matching peer name for this xfer\n");
			return;
		}

		/* reply with "PNAK" */
		trans = nateon_transaction_new(xfer->session->notification->cmdproc, "PNAK", "");
		nateon_cmdproc_send_trans(xfer->session->notification->cmdproc, trans);

		/* Open Listening P2P Port */
		xfer->p2p_listen_data = purple_network_listen_range(NATEON_P2P_START_PORT,\
				NATEON_P2P_END_PORT, SOCK_STREAM, p2p_listen_cb, xfer);

		xfer->p2p_cookie = g_strdup(params[2]);
		split = g_strsplit(params[1], ":", 2);
		xfer->p2p_connect_data = purple_proxy_connect(NULL, session->account, split[0], 
				atoi(split[1]), p2p_connect_cb, xfer);
		g_strfreev(split);
	}
	else if (param_count >= 4 && !strncmp(params[0], "FR", 2))
	{
		char **split;

		xfer = nateon_xfer_find_by_p2pcookie(session, params[2]);
		if (!xfer || xfer->conntype != NATEON_XFER_CONN_NONE)
		{
			return;
		}
		split = g_strsplit(params[1], ":", 2);
		xfer->fr_ip = g_strdup(split[0]);
		xfer->fr_port = atoi(split[1]);
		xfer->fr_authkey = g_strdup(params[3]);

		xfer->fr_connect_data = purple_proxy_connect(NULL, session->account, 
				xfer->fr_ip, xfer->fr_port, fr_connect_cb, xfer);

		g_strfreev(split);
	}
}

void
nateon_xfer_receive_file(NateonSession *session, NateonSwitchBoard *swboard, \
		const char *who, const char *filename, const int filesize, const char *cookie)
{
	NateonXfer *xfer;

	xfer = nateon_xfer_new(session, PURPLE_XFER_RECEIVE, who);

	xfer->content_type = NATEON_XFER_CONTENT_FILE;
	xfer->file_cookie = g_strdup(cookie);
	xfer->swboard = swboard;
	purple_xfer_set_filename(xfer->prpl_xfer, filename);
	purple_xfer_set_size(xfer->prpl_xfer, filesize);

	purple_xfer_set_init_fnc(xfer->prpl_xfer, nateon_xfer_init);
	purple_xfer_set_cancel_recv_fnc(xfer->prpl_xfer, nateon_xfer_cancel_recv);
	purple_xfer_set_request_denied_fnc(xfer->prpl_xfer, nateon_xfer_deny_request);
	purple_xfer_set_end_fnc(xfer->prpl_xfer, nateon_xfer_end);

	purple_xfer_request(xfer->prpl_xfer);
}

void
nateon_xfer_send_file(NateonSession *session, const char *who, const char *filename)
{
	NateonXfer *xfer;

	purple_debug_info("nateon", "%s: who:%s file:%s\n", __FUNCTION__, who, filename);

	xfer = nateon_xfer_new(session, PURPLE_XFER_SEND, who);
	xfer->content_type = NATEON_XFER_CONTENT_FILE;
	purple_xfer_set_init_fnc(xfer->prpl_xfer, nateon_xfer_init);
	purple_xfer_set_cancel_recv_fnc(xfer->prpl_xfer, nateon_xfer_cancel_recv_old);
	// purple_xfer_set_request_denied_fnc(xfer->prpl_xfer, nateon_xfer_request_denied);
	purple_xfer_set_end_fnc(xfer->prpl_xfer, nateon_xfer_end);
	purple_xfer_set_cancel_send_fnc(xfer->prpl_xfer, nateon_xfer_cancel_send);

	xfer->swboard = nateon_session_get_swboard(xfer->session, xfer->who, \
												NATEON_SB_FLAG_FT);

	if (filename)
	{
		purple_xfer_request_accepted(xfer->prpl_xfer, filename);
	} 
	else
	{
		purple_xfer_request(xfer->prpl_xfer);
	}
}

/**
 * Comapres the "signature" of two NateonXfers.
 */
gint xfer_sig_cmp( gconstpointer a, gconstpointer b )
{
	int t;
	const NateonXfer *da = a;
	const NateonXfer *db = b;
	gchar *fa, *fb;

	t = strcmp( da->who, db->who );
	if( t != 0 ) return t;

	// CANCEL request may have filename of NULL.
	fa = da->prpl_xfer->filename;
	fb = db->prpl_xfer->filename;
	if( fa && fb ) {
		t = strcmp( fa, fb );
		if( t != 0 ) return t;
	}

	t = strcmp( da->file_cookie, db->file_cookie );
	return t;
}

NateonXfer*
nateon_xfer_find_transfer(
	NateonSession *session, const char *who, const char *filename, const char *cookie)
{
	GList *node = NULL;
	NateonXfer *xfer = NULL;
	gchar **split;
	NateonXfer sig; // dummy
	PurpleXfer sig2; // dummy

	// 'who' might have dpkey. remove it.
	split = g_strsplit( who, "|", 2 );

	// data to find
	sig.prpl_xfer = &sig2;
	sig.who = split[0];
	sig.prpl_xfer->filename = g_strdup(filename); // Can do it /wo g_strdup
	sig.file_cookie = g_strdup(cookie); // but it emits compiler warning.

	node = g_list_find_custom( session->xfers, &sig, xfer_sig_cmp );
	if( node )
		xfer = node->data;

	g_strfreev(split);
	g_free( sig.prpl_xfer->filename );
	g_free( sig.file_cookie );

	return xfer;
}

void
nateon_xfer_request_denied(
	NateonSession *session, const char *who, const char *filename, const char *cookie)
{
	NateonXfer *xfer = nateon_xfer_find_transfer(session, who, filename, cookie);
	if (!xfer)
	{
		purple_debug_info("nateon", "%s: no matching xfer found for deny request\n", __FUNCTION__);
		return;
	}
	purple_xfer_cancel_remote( xfer->prpl_xfer );
	//purple_xfer_request_denied( xfer->prpl_xfer );
}

/**
 * When the sender canceled the request.
 */
void
nateon_xfer_cancel_transfer(NateonSession *session, const char *who, const char *filename, const char *cookie)
{
	NateonXfer *xfer;
	purple_debug_info("nateon", "%s\n", __FUNCTION__);
	xfer = nateon_xfer_find_transfer(session, who, filename, cookie);
	if (!xfer)
	{
		purple_debug_info("nateon", "%s: no matching xfer found for cancel request\n", __FUNCTION__);
		return;
	}
	purple_xfer_cancel_remote(xfer->prpl_xfer);
}

void
nateon_xfer_receive_buddyimage(NateonSession *session, NateonSwitchBoard *swboard, \
		const char *who, const char *uniq_name, const int filesize, const char *cookie)
{
	NateonXfer *xfer;

	xfer = nateon_xfer_new(session, PURPLE_XFER_RECEIVE, who);

	xfer->file_cookie = g_strdup(cookie);
	xfer->swboard = swboard;

	xfer->content_type = NATEON_XFER_CONTENT_BUDDYIMG;

	purple_xfer_set_filename(xfer->prpl_xfer, uniq_name);
	purple_xfer_set_size(xfer->prpl_xfer, filesize);
	purple_xfer_set_cancel_recv_fnc(xfer->prpl_xfer, nateon_xfer_cancel_recv_dummy);
	//purple_xfer_set_init_fnc(xfer->prpl_xfer, nateon_xfer_init);
	//purple_xfer_set_request_denied_fnc(xfer->prpl_xfer, nateon_xfer_request_denied);
	purple_xfer_set_end_fnc(xfer->prpl_xfer, nateon_xfer_end);

	nateon_xfer_init(xfer->prpl_xfer);
}

/*
PurpleXfer*
nateon_new_xfer(PurpleConnection *gc, const char *who)
{
    NateonXfer *xfer;

    purple_debug_info("nateon", "%s: who:%s\n", __FUNCTION__, who);

    xfer = nateon_xfer_new(gc->proto_data, PURPLE_XFER_SEND, who);
    xfer->content_type = NATEON_XFER_CONTENT_FILE;
    purple_xfer_set_init_fnc(xfer->prpl_xfer, nateon_xfer_init);
    purple_xfer_set_cancel_recv_fnc(xfer->prpl_xfer, nateon_xfer_cancel_recv);
    purple_xfer_set_request_denied_fnc(xfer->prpl_xfer, nateon_xfer_request_denied);
    purple_xfer_set_end_fnc(xfer->prpl_xfer, nateon_xfer_end);
    purple_xfer_set_cancel_send_fnc(xfer->prpl_xfer, nateon_xfer_cancel_send);

    xfer->swboard = nateon_session_get_swboard(xfer->session, xfer->who, NATEON_SB_FLAG_FT);
    return xfer->prpl_xfer;
}
*/
