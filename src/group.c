/**
 * @file group.c Group functions
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
#include "group.h"

NateonGroup *
nateon_group_new(NateonUserList *userlist, int id, const char *name)
{
	NateonGroup *group;

	g_return_val_if_fail(id >= 0,      NULL);
	g_return_val_if_fail(name != NULL, NULL);

	group = g_new0(NateonGroup, 1);

	nateon_userlist_add_group(userlist, group);

	group->id      = id;
	group->name    = g_strdup(name);

	return group;
}

void
nateon_group_destroy(NateonGroup *group)
{
	g_return_if_fail(group != NULL);

	g_free(group->name);
	g_free(group);
}

//void
//nateon_group_set_id(NateonGroup *group, int id)
//{
//	g_return_if_fail(group != NULL);
//	g_return_if_fail(id >= 0);
//
//	group->id = id;
//}
//
//void
//nateon_group_set_name(NateonGroup *group, const char *name)
//{
//	g_return_if_fail(group != NULL);
//	g_return_if_fail(name  != NULL);
//
//	if (group->name != NULL)
//		g_free(group->name);
//
//	group->name = g_strdup(name);
//}

int
nateon_group_get_id(const NateonGroup *group)
{
	g_return_val_if_fail(group != NULL, -1);

	return group->id;
}

const char *
nateon_group_get_name(const NateonGroup *group)
{
	g_return_val_if_fail(group != NULL, NULL);

	return group->name;
}
