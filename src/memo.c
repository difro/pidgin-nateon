/**
 * @file memo.c Paging functions
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
#include "memo.h"

NateonSop *
nateon_memo_new(const char *from, const char *to)
{
	NateonSop *memo;

	memo = g_new0(NateonSop, 1);

	memo->from = g_strdup(from);
	memo->to = g_strdup(to);

	return memo;
}

void
nateon_memo_destroy(NateonSop *memo)
{
	g_return_if_fail(memo != NULL);

	if (memo->body != NULL)
		g_free(memo->body);

	if (memo->from != NULL)
		g_free(memo->from);

	if (memo->to != NULL)
		g_free(memo->to);

	g_free(memo);
}

char *
nateon_memo_gen_payload(const NateonSop *memo, size_t *ret_size)
{
	const char *body;
	char *date;
	time_t now_t;
	struct tm *now;
	char *str;

	purple_debug_info("nateon", "[%s]\n", __FUNCTION__);

	g_return_val_if_fail(memo != NULL, NULL);

	body = nateon_memo_get_body(memo);

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
			memo->to, memo->from, memo->to, date, strlen(body), body);
	
	purple_debug_info("nateon", "[%s]\n%s\n", __FUNCTION__, str);

	if (ret_size != NULL)
		*ret_size = strlen(str);

	return str;
}

void
nateon_memo_set_body(NateonSop *memo, const char *body)
{
	g_return_if_fail(memo != NULL);
	g_return_if_fail(body != NULL);

	if (memo->body != NULL)
		g_free(memo->body);

	memo->body = g_strdup(body);
}

const char *
nateon_memo_get_body(const NateonSop *memo)
{
	g_return_val_if_fail(memo != NULL, NULL);

	return memo->body;
}
