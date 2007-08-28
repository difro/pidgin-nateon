/**
 * @file sop.c Paging functions
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
#include "sop.h"

NateonSop *
nateon_sop_new(const char *from, const char *to)
{
	NateonSop *sop;

	sop = g_new0(NateonSop, 1);

	sop->from = g_strdup(from);
	sop->to = g_strdup(to);

	return sop;
}

void
nateon_sop_destroy(NateonSop *sop)
{
	g_return_if_fail(sop != NULL);

	if (sop->body != NULL)
		g_free(sop->body);

	if (sop->from != NULL)
		g_free(sop->from);

	if (sop->to != NULL)
		g_free(sop->to);

	g_free(sop);
}

char *
nateon_sop_gen_payload(const NateonSop *sop, size_t *ret_size)
{
	const char *body;
	char *date;
	time_t now_t;
	struct tm *now;
	char *str;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	g_return_val_if_fail(sop != NULL, NULL);

	body = nateon_sop_get_body(sop);

	time(&now_t);
	now = localtime(&now_t);
	date = purple_utf8_strftime("%Y%m%d%H%M%S", now);

	//purple_debug_info("nateon", "[%s] date(%s)\n", __FUNCTION__, date);

	str = g_strdup_printf("%s\r\n"
			"IMSG\r\n"
			"title:title\r\n"
			"from:%s\r\n"
			"ref:%s\r\n"
			"date:%s\r\n"
			"contenttype:text\r\n"
			"length:%d\r\n"
			"\r\n"
			"%s\r\n",
			sop->to, sop->from, sop->to, date, strlen(body), body);
	
	purple_debug_info("nateon", "[%s]\n%s\n", __FUNCTION__, str);

	if (ret_size != NULL)
		*ret_size = strlen(str);

	return str;
}

void
nateon_sop_set_body(NateonSop *sop, const char *body)
{
	g_return_if_fail(sop != NULL);
	g_return_if_fail(body != NULL);

	if (sop->body != NULL)
		g_free(sop->body);

	sop->body = g_strdup(body);
}

const char *
nateon_sop_get_body(const NateonSop *sop)
{
	g_return_val_if_fail(sop != NULL, NULL);

	return sop->body;
}
