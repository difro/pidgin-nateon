/**
 * @file user.h User functions
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
#ifndef _NATEON_USER_H_
#define _NATEON_USER_H_

typedef struct _NateonUser  NateonUser;

#include "session.h"
//#include "object.h"

#include "userlist.h"

/**
 * A user.
 */
struct _NateonUser
{
#if 0
	NateonSession *session;    /**< The NATEON session.               */
#endif
	NateonUserList *userlist;

	char *id;		/**< The user ID.                   */
	char *account_name;     /**< The account name.              */
	char *store_name;       /**< The name stored in the server. */
	char *friendly_name;    /**< The friendly name.             */

	const char *status;     /**< The state of the user.         */
	gboolean idle;          /**< The idle state of the user.    */
//
//	struct
//	{
//		char *home;         /**< Home phone number.             */
//		char *work;         /**< Work phone number.             */
//		char *mobile;       /**< Mobile phone number.           */
//
//	} phone;
//
//	gboolean authorized;    /**< Authorized to add this user.   */
//	gboolean mobile;        /**< Signed up with NATEON Mobile.     */

	GList *group_ids;       /**< The group IDs.                 */

//	NateonObject *nateonobj;      /**< The user's NATEON Object.         */
//
//	GHashTable *clientcaps; /**< The client's capabilities.     */

	int list_op;
};

/**************************************************************************/
/** @name User API                                                        */
/**************************************************************************/
/*@{*/

/**
 * Creates a new user structure.
 *
 * @param session      The NATEON session.
 * @param account_name     The initial account_name.
 * @param stored_name  The initial stored name.
 *
 * @return A new user structure.
 */
NateonUser *nateon_user_new(NateonUserList *userlist, const char *account_name, const char *store_name, const char *id);

/**
 * Destroys a user structure.
 *
 * @param user The user to destroy.
 */
void nateon_user_destroy(NateonUser *user);


/**
 * Updates the user.
 *
 * Communicates with the core to update the ui, etc.
 *
 * @param user The user to update.
 */
void nateon_user_update(NateonUser *user);

/**
 * Sets the new state of user.
 *
 * @param user The user.
 * @param state The state string.
 */
void nateon_user_set_state(NateonUser *user, const char *state);

/**
 * Sets the account_name account for a user.
 *
 * @param user     The user.
 * @param account_name The account_name account.
 */
void nateon_user_set_account_name(NateonUser *user, const char *account_name);

/**
 * Sets the friendly name for a user.
 *
 * @param user The user.
 * @param name The friendly name.
 */
void nateon_user_set_friendly_name(NateonUser *user, const char *name);

/**
 * Sets the store name for a user.
 *
 * @param user The user.
 * @param name The store name.
 */
void nateon_user_set_store_name(NateonUser *user, const char *name);

/**
 * Sets the buddy icon for a local user.
 *
 * @param user     The user.
 * @param filename The path to the buddy icon.
 */
void nateon_user_set_buddy_icon(NateonUser *user, const char *filename);

///**
// * Sets the group ID list for a user.
// *
// * @param user The user.
// * @param ids  The group ID list.
// */
//void nateon_user_set_group_ids(NateonUser *user, GList *ids);

/**
 * Adds the group ID for a user.
 *
 * @param user The user.
 * @param id   The group ID.
 */
void nateon_user_add_group_id(NateonUser *user, int id);

/**
 * Removes the group ID from a user.
 *
 * @param user The user.
 * @param id   The group ID.
 */
void nateon_user_remove_group_id(NateonUser *user, int id);

///**
// * Sets the home phone number for a user.
// *
// * @param user   The user.
// * @param number The home phone number.
// */
//void nateon_user_set_home_phone(NateonUser *user, const char *number);
//
///**
// * Sets the work phone number for a user.
// *
// * @param user   The user.
// * @param number The work phone number.
// */
//void nateon_user_set_work_phone(NateonUser *user, const char *number);
//
///**
// * Sets the mobile phone number for a user.
// *
// * @param user   The user.
// * @param number The mobile phone number.
// */
//void nateon_user_set_mobile_phone(NateonUser *user, const char *number);
//
///**
// * Sets the NATEONObject for a user.
// *
// * @param user The user.
// * @param obj  The NATEONObject.
// */
//void nateon_user_set_object(NateonUser *user, NateonObject *obj);
//
///**
// * Sets the client information for a user.
// *
// * @param user The user.
// * @param info The client information.
// */
//void nateon_user_set_client_caps(NateonUser *user, GHashTable *info);


/**
 * Returns the account_name account for a user.
 *
 * @param user The user.
 *
 * @return The account_name account.
 */
const char *nateon_user_get_account_name(const NateonUser *user);

/**
 * Returns the friendly name for a user.
 *
 * @param user The user.
 *
 * @return The friendly name.
 */
const char *nateon_user_get_friendly_name(const NateonUser *user);

/**
 * Returns the store name for a user.
 *
 * @param user The user.
 *
 * @return The store name.
 */
const char *nateon_user_get_store_name(const NateonUser *user);

///**
// * Returns the home phone number for a user.
// *
// * @param user The user.
// *
// * @return The user's home phone number.
// */
//const char *nateon_user_get_home_phone(const NateonUser *user);
//
///**
// * Returns the work phone number for a user.
// *
// * @param user The user.
// *
// * @return The user's work phone number.
// */
//const char *nateon_user_get_work_phone(const NateonUser *user);
//
///**
// * Returns the mobile phone number for a user.
// *
// * @param user The user.
// *
// * @return The user's mobile phone number.
// */
//const char *nateon_user_get_mobile_phone(const NateonUser *user);
//
///**
// * Returns the NATEONObject for a user.
// *
// * @param user The user.
// *
// * @return The NATEONObject.
// */
//NateonObject *nateon_user_get_object(const NateonUser *user);
//
///**
// * Returns the client information for a user.
// *
// * @param user The user.
// *
// * @return The client information.
// */
//GHashTable *nateon_user_get_client_caps(const NateonUser *user);
//
///*@}*/

#endif /* _NATEON_USER_H_ */
