/**
 * @file nateon-utils.c Utility functions
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
#include "nateon-utils.h"

//void
//nateon_parse_format(const char *mime, char **pre_ret, char **post_ret)
char *
nateon_parse_format(const char *payload)
{
//	char *cur;
	char **split;
	GString *pre  = g_string_new(NULL);
	GString *post = g_string_new(NULL);
	unsigned int colors[3];
	char *fontface;
	char *fontcolor;
	char *fonteffect;
	char *body;
	char *tmp;

//	if (pre_ret  != NULL) *pre_ret  = NULL;
//	if (post_ret != NULL) *post_ret = NULL;

	purple_debug_info("nateon", "[%s], %s\n", __FUNCTION__, payload);

	split = g_strsplit(payload, "%09", 0);

	if (strstr(payload, "%09") == NULL)
	{
		fontface = g_strdup("굴림");
		fontcolor = g_strdup("0");
		fonteffect = g_strdup("");
		body = split[0];
	}
	else
	{
		fontface = split[0];
		fontcolor = g_strdup_printf("%06X", atoi(split[1]));
		fonteffect = split[2];
		body = split[3];
	}

	body = purple_strreplace(body, "%20", " ");
	tmp = body;

	body = purple_strreplace(tmp, "%0A", "<br>");
	g_free(tmp);
	tmp = body;

	body = purple_strreplace(tmp, "%25", "%");
	g_free(tmp);
	tmp = body;

	body = purple_strreplace(tmp, "%0D", "");
	g_free(tmp);

	purple_debug_info("nateon", "[%s], %s\n", __FUNCTION__, fontface);
	purple_debug_info("nateon", "[%s], %d\n", __FUNCTION__, fontcolor);
	purple_debug_info("nateon", "[%s], %s\n", __FUNCTION__, fonteffect);
	purple_debug_info("nateon", "[%s], %s\n", __FUNCTION__, body);
//	cur = strstr(mime, "FN=");
//
//	if (cur && (*(cur = cur + 3) != ';'))
	if (strcmp(fontface, ""))
	{
		pre = g_string_append(pre, "<FONT FACE=\"");
		pre = g_string_append(pre, fontface);
		pre = g_string_append(pre, "\">");
		post = g_string_prepend(post, "</FONT>");
	}

//	cur = strstr(mime, "EF=");

//	if (cur && (*(cur = cur + 3) != ';'))
	if (strcmp(fonteffect, ""))
	{
		char *effect = fonteffect;

		while (*effect)
		{
			pre = g_string_append_c(pre, '<');
			pre = g_string_append_c(pre, *effect);
			pre = g_string_append_c(pre, '>');
			post = g_string_prepend_c(post, '>');
			post = g_string_prepend_c(post, *effect);
			post = g_string_prepend_c(post, '/');
			post = g_string_prepend_c(post, '<');

			effect++;
		}
	}

//	cur = strstr(mime, "CO=");
//
//	if (cur && (*(cur = cur + 3) != ';'))
	if (strcmp(fontcolor, ""))
	{
		int i;

		i = sscanf(fontcolor, "%02x%02x%02x;", &colors[0], &colors[1], &colors[2]);

		if (i > 0)
		{
			char tag[64];

			if (i == 1)
			{
				colors[1] = 0;
				colors[2] = 0;
			}
			else if (i == 2)
			{
				unsigned int temp = colors[0];

				colors[0] = colors[1];
				colors[1] = temp;
				colors[2] = 0;
			}
			else if (i == 3)
			{
				unsigned int temp = colors[2];

				colors[2] = colors[0];
				colors[0] = temp;
			}

			g_snprintf(tag, sizeof(tag),
					   "<FONT COLOR=\"#%02hhx%02hhx%02hhx\">",
					   colors[0], colors[1], colors[2]);

			pre = g_string_append(pre, tag);
			post = g_string_prepend(post, "</FONT>");
		}
	}
//
//	cur = strstr(mime, "RL=");
//
//	if (cur && (*(cur = cur + 3) != ';'))
//	{
//		if (*cur == '1')
//		{
//			/* RTL text was received */
//			pre = g_string_append(pre, "<SPAN style=\"direction:rtl;text-align:right;\">");
//			post = g_string_prepend(post, "</SPAN>");
//		}
//	}
//
//	cur = g_strdup(purple_url_decode(pre->str));
//	g_string_free(pre, TRUE);
//
//	if (pre_ret != NULL)
//		*pre_ret = cur;
//	else
//		g_free(cur);
//
//	cur = g_strdup(purple_url_decode(post->str));
//	g_string_free(post, TRUE);
//
//	if (post_ret != NULL)
//		*post_ret = cur;
//	else
//		g_free(cur);

	purple_debug_info("nateon", "[%s] %s\n", __FUNCTION__, pre->str);
	purple_debug_info("nateon", "[%s] %s\n", __FUNCTION__, post->str);

	return g_strdup_printf("%s%s%s", g_string_free(pre, FALSE), body, g_string_free(post, FALSE));
}

/*
 * We need this because we're only supposed to encode spaces in the font
 * names. purple_url_encode() isn't acceptable.
 */
const char *
encode_spaces(const char *str)
{
	static char buf[BUF_LEN];
	const char *c;
	char *d;

	g_return_val_if_fail(str != NULL, NULL);

	for (c = str, d = buf; *c != '\0'; c++)
	{
		if (*c == ' ')
		{
			*d++ = '%';
			*d++ = '2';
			*d++ = '0';
		}
		else
			*d++ = *c;
	}

	return buf;
}

/*
 * Taken from the zephyr plugin.
 * This parses HTML formatting (put out by one of the gtkimhtml widgets
 * and converts it to nateon formatting. It doesn't deal with the tag closing,
 * but gtkimhtml widgets give valid html.
 * It currently deals properly with <b>, <u>, <i>, <font face=...>,
 * <font color=...>, <span dir=...>, <span style="direction: ...">.
 * It ignores <font back=...> and <font size=...>
 */
//void
//nateon_import_html(const char *html, char **attributes, char **message)
char *
nateon_import_html(const char *html)
{
	const char *c;
	GString *msg = g_string_new(NULL);
	char *fontface = NULL;
	char fonteffect[4];
//	char fontcolor[7];
	int fontcolor = 0;
//	char direction = '0';
	char *attributes;

	gboolean has_bold = FALSE;
	gboolean has_italic = FALSE;
	gboolean has_underline = FALSE;
	gboolean has_strikethrough = FALSE;

//	g_return_if_fail(html != NULL);

//	memset(fontcolor, 0, sizeof(fontcolor));
//	strcat(fontcolor, "0");
	memset(fonteffect, 0, sizeof(fonteffect));

	for (c = html; *c != '\0';)
	{
		if (*c == '%')
		{
			g_string_append(msg, "%25");
			c ++;
		}
		else if (*c == ' ' || *c == '\t')
		{
			g_string_append(msg, "%20");
			c ++;
		}
		else if (*c == '<')
		{
			if (!g_ascii_strncasecmp(c + 1, "br>", 3))
			{
				g_string_append(msg, "%0A");
				c += 4;
			}
			else if (!g_ascii_strncasecmp(c + 1, "b>", 2))
			{
				if (!has_bold)
				{
					strcat(fonteffect, "B");
					has_bold = TRUE;
				}
				c += 3;
			}
			else if (!g_ascii_strncasecmp(c + 1, "i>", 2))
			{
				if (!has_italic)
				{
					strcat(fonteffect, "I");
					has_italic = TRUE;
				}
				c += 3;
			}
			else if (!g_ascii_strncasecmp(c + 1, "s>", 2))
			{
				if (!has_strikethrough)
				{
					strcat(fonteffect, "S");
					has_strikethrough = TRUE;
				}
				c += 3;
			}
			else if (!g_ascii_strncasecmp(c + 1, "u>", 2))
			{
				if (!has_underline)
				{
					strcat(fonteffect, "U");
					has_underline = TRUE;
				}
				c += 3;
			}
//			else if (!g_ascii_strncasecmp(c + 1, "a href=\"", 8))
//			{
//				c += 9;
//
//				if (!g_ascii_strncasecmp(c, "mailto:", 7))
//					c += 7;
//
//				while ((*c != '\0') && g_ascii_strncasecmp(c, "\">", 2))
//					msg[retcount++] = *c++;
//
//				if (*c != '\0')
//					c += 2;
//
//				/* ignore descriptive string */
//				while ((*c != '\0') && g_ascii_strncasecmp(c, "</a>", 4))
//					c++;
//
//				if (*c != '\0')
//					c += 4;
//			}
//			else if (!g_ascii_strncasecmp(c + 1, "span", 4))
//			{
//				/* Bi-directional text support using CSS properties in span tags */
//				c += 5;
//
//				while (*c != '\0' && *c != '>')
//				{
//					while (*c == ' ')
//						c++;
//					if (!g_ascii_strncasecmp(c, "dir=\"rtl\"", 9))
//					{
//						c += 9;
//						direction = '1';
//					}
//					else if (!g_ascii_strncasecmp(c, "style=\"", 7))
//					{
//						/* Parse inline CSS attributes */
//						char *attributes;
//						int attr_len = 0;
//						c += 7;
//						while (*(c + attr_len) != '\0' && *(c + attr_len) != '"')
//							attr_len++;
//						if (*(c + attr_len) == '"')
//						{
//							char *attr_dir;
//							attributes = g_strndup(c, attr_len);
//							attr_dir = purple_markup_get_css_property(attributes, "direction");
//							if (attr_dir && (!g_ascii_strncasecmp(attr_dir, "RTL", 3)))
//								direction = '1';
//							g_free(attr_dir);
//							g_free(attributes);
//						}
//
//					}
//					else
//					{
//						c++;
//					}
//				}
//				if (*c == '>')
//					c++;
//			}
			else if (!g_ascii_strncasecmp(c + 1, "font", 4))
			{
				c += 5;

				while ((*c != '\0') && !g_ascii_strncasecmp(c, " ", 1))
					c++;

				if (!g_ascii_strncasecmp(c, "color=\"#", 7))
				{
					char color[7] = {0};

					c += 8;

					color[0] = *(c + 4);
					color[1] = *(c + 5);
					color[2] = *(c + 2);
					color[3] = *(c + 3);
					color[4] = *c;
					color[5] = *(c + 1);

					sscanf(color, "%06X", &fontcolor);

					c += 8;
				}
				else if (!g_ascii_strncasecmp(c, "face=\"", 6))
				{
					const char *end = NULL;
					const char *comma = NULL;
					unsigned int namelen = 0;

					c += 6;
					end = strchr(c, '\"');
					comma = strchr(c, ',');

					if (comma == NULL || comma > end)
						namelen = (unsigned int)(end - c);
					else
						namelen = (unsigned int)(comma - c);

					fontface = g_strndup(c, namelen);
					c = end + 2;
				}
				else
				{
					/* Drop all unrecognized/misparsed font tags */
					while ((*c != '\0') && g_ascii_strncasecmp(c, "\">", 2))
						c++;

					if (*c != '\0')
						c += 2;
				}
			}
			else
			{
				while ((*c != '\0') && (*c != '>'))
					c++;
				if (*c != '\0')
					c++;
			}
		}
		else if (*c == '&')
		{
			if (!g_ascii_strncasecmp(c, "&lt;", 4))
			{
				g_string_append_c(msg, '<');
				c += 4;
			}
			else if (!g_ascii_strncasecmp(c, "&gt;", 4))
			{
				g_string_append_c(msg, '>');
				c += 4;
			}
			else if (!g_ascii_strncasecmp(c, "&nbsp;", 6))
			{
				g_string_append(msg, "%20");
				c += 6;
			}
			else if (!g_ascii_strncasecmp(c, "&quot;", 6))
			{
				g_string_append_c(msg, '"');
				c += 6;
			}
			else if (!g_ascii_strncasecmp(c, "&amp;", 5))
			{
				g_string_append_c(msg, '&');
				c += 5;
			}
			else if (!g_ascii_strncasecmp(c, "&apos;", 6))
			{
				g_string_append_c(msg, '\'');
				c += 6;
			}
			else
				g_string_append_c(msg, *c++);
		}
		else
			g_string_append_c(msg, *c++);
	}

	if (fontface == NULL)
		fontface = g_strdup("굴림");

	purple_debug_info("nateon", "FN=%s; EF=%s; CO=%d\n",
			fontface, fonteffect, fontcolor);

	attributes = g_strdup_printf("%s%%09%d%%09%s%%09",
		       	encode_spaces(fontface), fontcolor, fonteffect);
	g_string_prepend(msg, attributes);

	g_free(fontface);
	g_free(attributes);

	return g_string_free(msg, FALSE);
}

//void
//nateon_parse_socket(const char *str, char **ret_host, int *ret_port)
//{
//	char *host;
//	char *c;
//	int port;
//
//	host = g_strdup(str);
//
//	if ((c = strchr(host, ':')) != NULL)
//	{
//		*c = '\0';
//		port = atoi(c + 1);
//	}
//	else
//		port = 1863;
//
//	*ret_host = host;
//	*ret_port = port;
//}
