/**
 * @file user.c User functions
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
#include "user.h"
//#include "slp.h"

NateonUser *nateon_user_new(NateonUserList *userlist, const char *account_name, const char *store_name, const char *id)
{
	NateonUser *user;

	user = g_new0(NateonUser, 1);

	user->userlist = userlist;
	user->id = g_strdup(id);

	nateon_user_set_account_name(user, account_name);
	nateon_user_set_store_name(user, store_name);

	/*
	 * XXX This seems to reset the friendly name from what it should be
	 *     to the account_name when moving users. So, screw it :)
	 */
//#if 0
//	if (name != NULL)
//		nateon_user_set_name(user, name);
//#endif

	return user;
}

void
nateon_user_destroy(NateonUser *user)
{
	g_return_if_fail(user != NULL);

//	if (user->clientcaps != NULL)
//		g_hash_table_destroy(user->clientcaps);

	if (user->group_ids != NULL)
		g_list_free(user->group_ids);

//	if (user->nateonobj != NULL)
//		nateon_object_destroy(user->nateonobj);

	if (user->account_name != NULL)
		g_free(user->account_name);

	if (user->friendly_name != NULL)
		g_free(user->friendly_name);

	if (user->store_name != NULL)
		g_free(user->store_name);

//	if (user->phone.home != NULL)
//		g_free(user->phone.home);
//
//	if (user->phone.work != NULL)
//		g_free(user->phone.work);
//
//	if (user->phone.mobile != NULL)
//		g_free(user->phone.mobile);

	g_free(user);
}

void
nateon_user_update(NateonUser *user)
{
	PurpleAccount *account;

	account = user->userlist->session->account;

	if (user->status != NULL)
	{
		if(!strcmp(user->status, "F"))
			purple_prpl_got_user_status(account, user->account_name, "offline", NULL);
		else
			purple_prpl_got_user_status(account, user->account_name, user->status, NULL);
	}

	if (user->idle)
		purple_prpl_got_user_idle(account, user->account_name, TRUE, -1);
	else
		purple_prpl_got_user_idle(account, user->account_name, FALSE, 0);
}

void
nateon_user_set_state(NateonUser *user, const char *state)
{
	if (!g_ascii_strcasecmp(state, "IDL"))
		user->idle = TRUE;
	else
		user->idle = FALSE;

	user->status = g_strdup(state);
}

void nateon_user_set_account_name(NateonUser *user, const char *account_name)
{
	g_return_if_fail(user != NULL);

	if (user->account_name != NULL)
		g_free(user->account_name);

	user->account_name = g_strdup(account_name);
}

void nateon_user_set_friendly_name(NateonUser *user, const char *name)
{
	g_return_if_fail(user != NULL);

	if (user->friendly_name != NULL)
		g_free(user->friendly_name);

	user->friendly_name = g_strdup(name);
}

void nateon_user_set_store_name(NateonUser *user, const char *name)
{
	g_return_if_fail(user != NULL);

	if (user->store_name != NULL)
		g_free(user->store_name);

	user->store_name = g_strdup(name);
}

void
nateon_user_set_buddy_icon(NateonUser *user, PurpleStoredImage *img)
{
//	struct stat st;
//	FILE *fp;
//	NateonObject *nateonobj = nateon_user_get_object(user);

	g_return_if_fail(user != NULL);

//	if (filename == NULL || g_stat(filename, &st) == -1)
//	{
//		nateon_user_set_object(user, NULL);
//	}
//	else if ((fp = g_fopen(filename, "rb")) != NULL)
//	{
//		PurpleCipherContext *ctx;
//		char *buf;
//		gsize len;
//		char *base64;
//		unsigned char digest[20];
//
//		if (nateonobj == NULL)
//		{
//			nateonobj = nateon_object_new();
//			nateon_object_set_local(nateonobj);
//			nateon_object_set_type(nateonobj, NATEON_OBJECT_USERTILE);
//			nateon_object_set_location(nateonobj, "TFR2C2.tmp");
//			nateon_object_set_creator(nateonobj, nateon_user_get_account_name(user));
//
//			nateon_user_set_object(user, nateonobj);
//		}
//
//		nateon_object_set_real_location(nateonobj, filename);
//
//		buf = g_malloc(st.st_size);
//		len = fread(buf, 1, st.st_size, fp);
//
//		fclose(fp);
//
//		/* Compute the SHA1D field. */
//		memset(digest, 0, sizeof(digest));
//
//		ctx = purple_cipher_context_new_by_name("sha1", NULL);
//		purple_cipher_context_append(ctx, (const guchar *)buf, st.st_size);
//		purple_cipher_context_digest(ctx, sizeof(digest), digest, NULL);
//		g_free(buf);
//
//		base64 = purple_base64_encode(digest, sizeof(digest));
//		nateon_object_set_sha1d(nateonobj, base64);
//		g_free(base64);
//
//		nateon_object_set_size(nateonobj, st.st_size);
//
//		/* Compute the SHA1C field. */
//		buf = g_strdup_printf(
//			"Creator%sSize%dType%dLocation%sFriendly%sSHA1D%s",
//			nateon_object_get_creator(nateonobj),
//			nateon_object_get_size(nateonobj),
//			nateon_object_get_type(nateonobj),
//			nateon_object_get_location(nateonobj),
//			nateon_object_get_friendly(nateonobj),
//			nateon_object_get_sha1d(nateonobj));
//
//		memset(digest, 0, sizeof(digest));
//
//		purple_cipher_context_reset(ctx, NULL);
//		purple_cipher_context_append(ctx, (const guchar *)buf, strlen(buf));
//		purple_cipher_context_digest(ctx, sizeof(digest), digest, NULL);
//		purple_cipher_context_destroy(ctx);
//		g_free(buf);
//
//		base64 = purple_base64_encode(digest, sizeof(digest));
//		nateon_object_set_sha1c(nateonobj, base64);
//		g_free(base64);
//	}
//	else
//	{
//		purple_debug_error("nateon", "Unable to open buddy icon %s!\n", filename);
//		nateon_user_set_object(user, NULL);
//	}
}

void
nateon_user_add_group_id(NateonUser *user, int id)
{
	NateonUserList *userlist;
	PurpleAccount *account;
	PurpleBuddy *b;
	PurpleGroup *g;
	const char *account_name;
	const char *group_name;

	g_return_if_fail(user != NULL);
	g_return_if_fail(id >= 0);

	if (g_list_find(user->group_ids, GINT_TO_POINTER(id)) == NULL)
		user->group_ids = g_list_append(user->group_ids, GINT_TO_POINTER(id));

	userlist = user->userlist;
	account = userlist->session->account;
	account_name = nateon_user_get_account_name(user);

	group_name = nateon_userlist_find_group_name(userlist, id);

	g = purple_find_group(group_name);

	if (g == NULL)
	{
		g = purple_group_new(group_name);
		purple_blist_add_group(g, NULL);
	}

	b = purple_find_buddy_in_group(account, account_name, g);

	if (b == NULL)
	{
		b = purple_buddy_new(account, account_name, NULL);

		purple_blist_add_buddy(b, NULL, g, NULL);
	}

	b->proto_data = user;
}

void
nateon_user_remove_group_id(NateonUser *user, int id)
{
	g_return_if_fail(user != NULL);
	g_return_if_fail(id >= 0);

	user->group_ids = g_list_remove(user->group_ids, GINT_TO_POINTER(id));
}

//void
//nateon_user_set_home_phone(NateonUser *user, const char *number)
//{
//	g_return_if_fail(user != NULL);
//
//	if (user->phone.home != NULL)
//		g_free(user->phone.home);
//
//	user->phone.home = (number == NULL ? NULL : g_strdup(number));
//}
//
//void
//nateon_user_set_work_phone(NateonUser *user, const char *number)
//{
//	g_return_if_fail(user != NULL);
//
//	if (user->phone.work != NULL)
//		g_free(user->phone.work);
//
//	user->phone.work = (number == NULL ? NULL : g_strdup(number));
//}
//
//void
//nateon_user_set_mobile_phone(NateonUser *user, const char *number)
//{
//	g_return_if_fail(user != NULL);
//
//	if (user->phone.mobile != NULL)
//		g_free(user->phone.mobile);
//
//	user->phone.mobile = (number == NULL ? NULL : g_strdup(number));
//}
//
//void
//nateon_user_set_object(NateonUser *user, NateonObject *obj)
//{
//	g_return_if_fail(user != NULL);
//
//	if (user->nateonobj != NULL)
//		nateon_object_destroy(user->nateonobj);
//
//	user->nateonobj = obj;
//
//	if (user->list_op & NATEON_LIST_FL_OP)
//		nateon_queue_buddy_icon_request(user);
//}
//
//void
//nateon_user_set_client_caps(NateonUser *user, GHashTable *info)
//{
//	g_return_if_fail(user != NULL);
//	g_return_if_fail(info != NULL);
//
//	if (user->clientcaps != NULL)
//		g_hash_table_destroy(user->clientcaps);
//
//	user->clientcaps = info;
//}

const char *nateon_user_get_account_name(const NateonUser *user)
{
	g_return_val_if_fail(user != NULL, NULL);

	return user->account_name;
}

const char *
nateon_user_get_friendly_name(const NateonUser *user)
{
	g_return_val_if_fail(user != NULL, NULL);

	return user->friendly_name;
}

const char *nateon_user_get_store_name(const NateonUser *user)
{
	g_return_val_if_fail(user != NULL, NULL);

	return user->store_name;
}

//const char *
//nateon_user_get_home_phone(const NateonUser *user)
//{
//	g_return_val_if_fail(user != NULL, NULL);
//
//	return user->phone.home;
//}
//
//const char *
//nateon_user_get_work_phone(const NateonUser *user)
//{
//	g_return_val_if_fail(user != NULL, NULL);
//
//	return user->phone.work;
//}
//
//const char *
//nateon_user_get_mobile_phone(const NateonUser *user)
//{
//	g_return_val_if_fail(user != NULL, NULL);
//
//	return user->phone.mobile;
//}
//
//NateonObject *
//nateon_user_get_object(const NateonUser *user)
//{
//	g_return_val_if_fail(user != NULL, NULL);
//
//	return user->nateonobj;
//}
//
//GHashTable *
//nateon_user_get_client_caps(const NateonUser *user)
//{
//	g_return_val_if_fail(user != NULL, NULL);
//
//	return user->clientcaps;
//}
