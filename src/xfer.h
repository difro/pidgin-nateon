/**
 * @file xfer.h NATEON file transfer functions
 * @ingroup nateon 
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
#ifndef _NATEON_XFER_H_
#define _NATEON_XFER_H_

#include "libpurple/network.h"

#define NATEON_XFER_SEND_BUFFER_SIZE	(8191)

typedef struct _NateonXfer NateonXfer;
typedef struct _NateonXferConnection NateonXferConnection;

/**
 * P2P Connection for file transfer.
 */
struct _NateonXferConnection {
	int fd;		/**< socket file descriptor */

	int rx_pa;		/**< socket read watcher */
	char *rx_buf;	/**< The receive buffer. */
	int rx_len;		/**< The receive buffer length. */

	PurpleCircBuffer *tx_buf;
	gint tx_handler;

	int inpa;

	NateonXfer *nate_xfer;
};

typedef enum
{
	NATEON_XFER_CONN_NONE,
	NATEON_XFER_CONN_P2P,
	NATEON_XFER_CONN_FR
} NateonXferConnType;

typedef enum
{
    NATEON_XFER_CONTENT_FILE,
    NATEON_XFER_CONTENT_BUDDYIMG
} NateonXferContentType;

/**
 * File Transfer.
 */
struct _NateonXfer
{
	NateonSession *session; /**< The NATEON session of this file transfer */
	NateonSwitchBoard *swboard;

	PurpleXfer *prpl_xfer;

	char *who;	/**< peer's account name */
	char *dpkey;
	char *my_ip;

    NateonXferConnType content_type;

	/* P2P related */
	PurpleNetworkListenData	*p2p_listen_data;
	PurpleProxyConnectData	*p2p_connect_data;
	int		p2p_listen_pa;		/**< listen port watcher */
	int		p2p_listen_fd;
	char	*p2p_cookie;
	int		p2p_listen_port;	/**< listening port number */
	guint	p2p_timer;

	/* FR-server related */
	int fr_initiate_trid;
	char *fr_ip;
	int fr_port;
	char *fr_authkey;
	PurpleProxyConnectData *fr_connect_data;

	char *file_cookie;	/**< cookie of the file transferred */
	FILE *dest_fp;

	NateonXferConnType conntype;

	NateonXferConnection conn;

	gboolean chunk_processing;
	int chunk_cur_len;
	int chunk_len;

	int recv_len;

	FILE *local_fp;
	char send_buf[NATEON_XFER_SEND_BUFFER_SIZE];
	int send_data_trid;
	int sent_len;
};

/**
 * Start File-receive process.
 *
 * @param session Nateon Session
 * @param who file sender account, with dpkey appended.
 * @param filename filename
 * @param filesize size of the file receiving
 * @param cookie file cookie
 */
void nateon_xfer_receive_file(NateonSession *session, NateonSwitchBoard *swboard, \
		const char *who, const char *filename, const int filesize, const char *cookie);

/**
 * Start File-send process.
 *
 * @param session Nateon Session
 * @param who file receiver account
 * @param filename filename
 */
void nateon_xfer_send_file(NateonSession *session, const char *who, const char *filename);

void nateon_xfer_parse_reqc(NateonSession *session, NateonCmdProc *cmdproc, char **params, \
								int param_count);

void nateon_xfer_parse_refr(NateonSession *session, char **params, int param_count);

NateonXfer* nateon_xfer_find_transfer(NateonSession *session, const char *who, const char *filename,\
									const char *cookie);

void nateon_xfer_cancel_transfer(NateonSession *session, const char *who, const char *filename,\
									const char *cookie);

void nateon_xfer_request_denied(NateonSession *session, const char *who, const char *filename,\
									const char *cookie);

void nateon_xfer_receive_buddyimage(NateonSession *session, NateonSwitchBoard *swboard, \
		const char *who, const char *uniq_name, const int filesize, const char *cookie);

//PurpleXfer* nateon_new_xfer(PurpleConnection *gc, const char *who);
#endif /* _NATEON_XFER_H_ */
