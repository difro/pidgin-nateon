/**
 * @file servconn.h Server connection functions
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
#ifndef _NATEON_SERVCONN_H_
#define _NATEON_SERVCONN_H_

typedef struct _NateonServConn NateonServConn;

#include "session.h"
#include "cmdproc.h"

#include "proxy.h"
//#include "httpconn.h"
#include "circbuffer.h"

/**
 * Connection error types.
 */
typedef enum
{
	NATEON_SERVCONN_ERROR_NONE,
	NATEON_SERVCONN_ERROR_CONNECT,
	NATEON_SERVCONN_ERROR_WRITE,
	NATEON_SERVCONN_ERROR_READ,

} NateonServConnError;

/**
 * Connection types.
 */
typedef enum
{
	NATEON_SERVCONN_NS,
	NATEON_SERVCONN_SB

} NateonServConnType;

/**
 * A Connection.
 */
struct _NateonServConn
{
	NateonServConnType type; /**< The type of this connection. */
	NateonSession *session;  /**< The NATEON session of this connection. */
	NateonCmdProc *cmdproc;  /**< The command processor of this connection. */

	PurpleProxyConnectData *connect_data;

	gboolean connected;   /**< A flag that states if it's connected. */
	gboolean processing;  /**< A flag that states if something is working
							with this connection. */
	gboolean wasted;      /**< A flag that states if it should be destroyed. */

	char *host; /**< The host this connection is connected or should be
				  connected to. */
	int num; /**< A number id of this connection. */

//	NateonHttpConn *httpconn; /**< The HTTP connection this connection should use. */

	int fd; /**< The connection's file descriptor. */
	int inpa; /**< The connection's input handler. */

	char *rx_buf; /**< The receive buffer. */
	int rx_len; /**< The receive buffer lenght. */

	size_t payload_len; /**< The length of the payload.
						  It's only set when we've received a command that
						  has a payload. */

	PurpleCircBuffer *tx_buf;
	guint tx_handler;

	void (*connect_cb)(NateonServConn *); /**< The callback to call when connecting. */
	void (*disconnect_cb)(NateonServConn *); /**< The callback to call when disconnecting. */
	void (*destroy_cb)(NateonServConn *); /**< The callback to call when destroying. */
};

/**
 * Creates a new connection object.
 *
 * @param session The session.
 * @param type The type of the connection.
 */
NateonServConn *nateon_servconn_new(NateonSession *session, NateonServConnType type);

/**
 * Destroys a connection object.
 *
 * @param servconn The connection.
 */
void nateon_servconn_destroy(NateonServConn *servconn);

/**
 * Connects to a host.
 *
 * @param servconn The connection.
 * @param host The host.
 * @param port The port.
 */
gboolean nateon_servconn_connect(NateonServConn *servconn, const char *host, int port);

/**
 * Disconnects.
 *
 * @param servconn The connection.
 */
void nateon_servconn_disconnect(NateonServConn *servconn);

/**
 * Sets the connect callback.
 *
 * @param servconn The servconn.
 * @param connect_cb The connect callback.
 */
void nateon_servconn_set_connect_cb(NateonServConn *servconn,
								 void (*connect_cb)(NateonServConn *));
/**
 * Sets the disconnect callback.
 *
 * @param servconn The servconn.
 * @param disconnect_cb The disconnect callback.
 */
void nateon_servconn_set_disconnect_cb(NateonServConn *servconn,
									void (*disconnect_cb)(NateonServConn *));
/**
 * Sets the destroy callback.
 *
 * @param servconn The servconn that's being destroyed.
 * @param destroy_cb The destroy callback.
 */
void nateon_servconn_set_destroy_cb(NateonServConn *servconn,
								 void (*destroy_cb)(NateonServConn *));

/**
 * Writes a chunck of data to the servconn.
 *
 * @param servconn The servconn.
 * @param buf The data to write.
 * @param size The size of the data.
 */
ssize_t nateon_servconn_write(NateonServConn *servconn, const char *buf,
						  size_t size);

/**
 * Function to call whenever an error related to a switchboard occurs.
 *
 * @param servconn The servconn.
 * @param error The error that happened.
 */
void nateon_servconn_got_error(NateonServConn *servconn, NateonServConnError error);

#endif /* _NATEON_SERVCONN_H_ */
