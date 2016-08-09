/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  GMime
 *  Copyright (C) 2000-2014 Jeffrey Stedfast
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gtrie.h"
#include "url-scanner.h"


struct _UrlScanner {
	GPtrArray *patterns;
	GTrie *trie;
};


UrlScanner *
url_scanner_new (void)
{
	UrlScanner *scanner;
	
	scanner = g_new (UrlScanner, 1);
	scanner->patterns = g_ptr_array_new ();
	scanner->trie = g_trie_new (TRUE);
	
	return scanner;
}


void
url_scanner_free (UrlScanner *scanner)
{
	g_return_if_fail (scanner != NULL);
	
	g_ptr_array_free (scanner->patterns, TRUE);
	g_trie_free (scanner->trie);
	g_free (scanner);
}


void
url_scanner_add (UrlScanner *scanner, urlpattern_t *pattern)
{
	g_return_if_fail (scanner != NULL);
	
	g_trie_add (scanner->trie, pattern->pattern, scanner->patterns->len);
	g_ptr_array_add (scanner->patterns, pattern);
}


gboolean
url_scanner_scan (UrlScanner *scanner, const char *in, size_t inlen, urlmatch_t *match)
{
	const char *pos, *inend;
	urlpattern_t *pat;
	int pattern_id;
	
	g_return_val_if_fail (scanner != NULL, FALSE);
	g_return_val_if_fail (in != NULL, FALSE);
	
	if (!(pos = g_trie_search (scanner->trie, in, inlen, &pattern_id)))
		return FALSE;
	
	pat = g_ptr_array_index (scanner->patterns, pattern_id);
	
	match->pattern = pat->pattern;
	match->prefix = pat->prefix;
	
	inend = in + inlen;
	if (!pat->start (in, pos, inend, match))
		return FALSE;
	
	if (!pat->end (in, pos, inend, match))
		return FALSE;
	
	return TRUE;
}


static unsigned char url_scanner_table[256] = {
	  1,  1,  1,  1,  1,  1,  1,  1,  1,  9,  9,  1,  1,  9,  1,  1,
	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	 24,128,160,128,128,128,128,128,160,160,128,128,160,192,160,160,
	 68, 68, 68, 68, 68, 68, 68, 68, 68, 68,160,160, 32,128, 32,128,
	160, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,160,160,160,128,128,
	128, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
	 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,128,128,128,128,  1,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128
};

enum {
	IS_CTRL		= (1 << 0),
	IS_ALPHA        = (1 << 1),
	IS_DIGIT        = (1 << 2),
	IS_LWSP		= (1 << 3),
	IS_SPACE	= (1 << 4),
	IS_SPECIAL	= (1 << 5),
	IS_DOMAIN       = (1 << 6),
	IS_URLSAFE      = (1 << 7),
};

#define is_ctrl(x) ((url_scanner_table[(unsigned char)(x)] & IS_CTRL) != 0)
#define is_lwsp(x) ((url_scanner_table[(unsigned char)(x)] & IS_LWSP) != 0)
#define is_atom(x) ((url_scanner_table[(unsigned char)(x)] & (IS_SPECIAL|IS_SPACE|IS_CTRL)) == 0)
#define is_alpha(x) ((url_scanner_table[(unsigned char)(x)] & IS_ALPHA) != 0)
#define is_digit(x) ((url_scanner_table[(unsigned char)(x)] & IS_DIGIT) != 0)
#define is_domain(x) ((url_scanner_table[(unsigned char)(x)] & IS_DOMAIN) != 0)
#define is_urlsafe(x) ((url_scanner_table[(unsigned char)(x)] & (IS_ALPHA|IS_DIGIT|IS_URLSAFE)) != 0)

static struct {
	char open;
	char close;
} url_braces[] = {
	{ '(', ')' },
	{ '{', '}' },
	{ '[', ']' },
	{ '<', '>' },
	{ '|', '|' },
};

static gboolean
is_open_brace (char c)
{
	unsigned int i;
	
	for (i = 0; i < G_N_ELEMENTS (url_braces); i++) {
		if (c == url_braces[i].open)
			return TRUE;
	}
	
	return FALSE;
}

static char
url_stop_at_brace (const char *in, size_t so)
{
	int i;
	
	if (so > 0) {
		for (i = 0; i < 4; i++) {
			if (in[so - 1] == url_braces[i].open)
				return url_braces[i].close;
		}
	}
	
	return '\0';
}


gboolean
url_addrspec_start (const char *in, const char *pos, const char *inend, urlmatch_t *match)
{
  (void) (inend);
	register const char *inptr = pos;
	
	g_assert (*inptr == '@');
	
	if (inptr == in)
		return FALSE;
	
	inptr--;
	
	while (inptr > in) {
		if (is_atom (*inptr))
			inptr--;
		else
			break;
		
		while (inptr > in && is_atom (*inptr))
			inptr--;
		
		if (inptr > in && *inptr == '.')
			inptr--;
	}
	
	if (!is_atom (*inptr) || is_open_brace (*inptr))
		inptr++;
	
	if (inptr == pos)
		return FALSE;
	
	match->um_so = (inptr - in);
	
	return TRUE;
}

gboolean
url_addrspec_end (const char *in, const char *pos, const char *inend, urlmatch_t *match)
{
	const char *inptr = pos;
	int parts = 0, digits;
	gboolean got_dot = FALSE;
	
	g_assert (*inptr == '@');
	
	inptr++;
	
	if (*inptr == '[') {
		/* domain literal */
		do {
			inptr++;
			
			digits = 0;
			while (inptr < inend && is_digit (*inptr) && digits < 3) {
				inptr++;
				digits++;
			}
			
			parts++;
			
			if (*inptr != '.' && parts != 4)
				return FALSE;
		} while (parts < 4);
		
		if (inptr < inend && *inptr == ']')
			inptr++;
		else
			return FALSE;
		
		got_dot = TRUE;
	} else {
		while (inptr < inend) {
			if (is_domain (*inptr))
				inptr++;
			else
				break;
			
			while (inptr < inend && is_domain (*inptr))
				inptr++;
			
			if (inptr < inend && *inptr == '.' && is_domain (inptr[1])) {
				if (*inptr == '.')
					got_dot = TRUE;
				inptr++;
			}
		}
	}
	
	if (inptr == pos + 1 || !got_dot)
		return FALSE;
	
	match->um_eo = (inptr - in);
	
	return TRUE;
}

gboolean url_id_end (const char *in, const char *pos, const char *inend, urlmatch_t *match) {
  char * alpha = (char*) pos;

  while (alpha < inend) {
    if (*alpha == '@') return url_addrspec_end (in, alpha, inend, match);
    alpha++;
  }

  return FALSE;
}

gboolean
url_file_start (const char *in, const char *pos, const char *inend, urlmatch_t *match)
{
  (void) (inend);
	match->um_so = (pos - in);
	
	return TRUE;
}

gboolean
url_file_end (const char *in, const char *pos, const char *inend, urlmatch_t *match)
{
	register const char *inptr = pos;
	char close_brace;
	
	inptr += strlen (match->pattern);
	
	if (*inptr == '/')
		inptr++;
	
	close_brace = url_stop_at_brace (in, match->um_so);
	
	while (inptr < inend && is_urlsafe (*inptr) && *inptr != close_brace)
		inptr++;
	
	if (inptr == pos)
		return FALSE;
	
	match->um_eo = (inptr - in);
	
	return TRUE;
}

gboolean
url_web_start (const char *in, const char *pos, const char *inend, urlmatch_t *match)
{
  (void) (inend);
	match->um_so = (pos - in);
	
	return TRUE;
}

gboolean
url_web_end (const char *in, const char *pos, const char *inend, urlmatch_t *match)
{
	register const char *inptr = pos;
	gboolean openbracket = FALSE;
	gboolean passwd = FALSE;
	const char *save;
	char close_brace;
	int port, val, n;
	char *end;
	
	inptr += strlen (match->pattern);
	
	close_brace = url_stop_at_brace (in, match->um_so);
	
	/* find the end of the domain */
	if (is_digit (*inptr)) {
		goto ip_literal2;
	} else if (is_atom (*inptr)) {
		/* might be a domain or user@domain */
		save = inptr;
		while (inptr < inend) {
			if (!is_atom (*inptr))
				break;
			
			inptr++;
			
			while (inptr < inend && is_atom (*inptr))
				inptr++;
			
			if ((inptr + 1) < inend && *inptr == '.' && is_atom (inptr[1]))
				inptr++;
		}
		
		if (*inptr != '@')
			inptr = save;
		else
			inptr++;
		
		if (*inptr == '[') {
			/* IPv6 (or possibly IPv4) address literal */
			goto ip_literal;
		}
		
		if (is_domain (*inptr)) {
			/* domain name or IPv4 address */
			goto domain;
		}
	} else if (*inptr == '[') {
	ip_literal:
		openbracket = TRUE;
		inptr++;
		
		if (is_digit (*inptr)) {
		ip_literal2:
			/* could be IPv4 or IPv6 */
			if ((val = strtol (inptr, &end, 10)) < 0)
				return FALSE;
		} else if ((*inptr >= 'A' && *inptr <= 'F') || (*inptr >= 'a' && *inptr <= 'f')) {
			/* IPv6 address literals are in hex */
			if ((val = strtol (inptr, &end, 16)) < 0 || *end != ':')
				return FALSE;
		} else if (*inptr == ':') {
			/* IPv6 can start with a ':' */
			end = (char *) inptr;
			val = 256; /* invalid value */
		} else {
			return FALSE;
		}
		
		switch (*end) {
		case '.': /* IPv4 address literal */
			n = 1;
			do {
				if (val > 255 || *end != '.')
					return FALSE;
				
				inptr = end + 1;
				if ((val = strtol (inptr, &end, 10)) < 0)
					return FALSE;
				
				n++;
			} while (n < 4);
			
			if (val > 255 || n < 4 || (openbracket && *end != ']'))
				return FALSE;
			
			inptr = end + 1;
			break;
		case ':': /* IPv6 address literal */
			if (!openbracket)
				return FALSE;
			
			do {
				if (end[1] != ':') {
					inptr = end + 1;
					if ((val = strtol (inptr, &end, 16)) < 0)
						return FALSE;
				} else {
					inptr = end;
					end++;
				}
			} while (end > inptr && *end == ':');
			
			if (*end != ']')
				return FALSE;
			
			inptr = end + 1;
			break;
		default:
			return FALSE;
		}
	} else if (is_domain (*inptr)) {
	domain:
		while (inptr < inend) {
			if (!is_domain (*inptr))
				break;
			
			inptr++;
			
			while (inptr < inend && is_domain (*inptr))
				inptr++;
			
			if ((inptr + 1) < inend && *inptr == '.' && (is_domain (inptr[1]) || inptr[1] == '/'))
				inptr++;
		}
	} else {
		return FALSE;
	}
	
	if (inptr < inend) {
		switch (*inptr) {
		case ':': /* we either have a port or a password */
			inptr++;
			
			if (is_digit (*inptr) || passwd) {
				port = (*inptr++ - '0');
				
				while (inptr < inend && is_digit (*inptr) && port < 65536)
					port = (port * 10) + (*inptr++ - '0');
				
				if (!passwd && (port >= 65536 || *inptr == '@')) {
					if (inptr < inend) {
						/* this must be a password? */
						goto passwd;
					}
					
					inptr--;
				}
			} else {
			passwd:
				passwd = TRUE;
				save = inptr;
				
				while (inptr < inend && is_atom (*inptr))
					inptr++;
				
				if ((inptr + 2) < inend) {
					if (*inptr == '@') {
						inptr++;
						if (is_domain (*inptr))
							goto domain;
					}
					
					return FALSE;
				}
			}
			
			if (inptr >= inend || *inptr != '/')
				break;
			
			/* we have a '/' so there could be a path - fall through */
		case '/': /* we've detected a path component to our url */
			inptr++;
			
			while (inptr < inend && is_urlsafe (*inptr) && *inptr != close_brace)
				inptr++;
			
			break;
		default:
			break;
		}
	}
	
	/* urls are extremely unlikely to end with any
	 * punctuation, so strip any trailing
	 * punctuation off. Also strip off any closing
	 * braces or quotes. */
	while (inptr > pos && strchr (",.:;?!-|)}]'\"", inptr[-1]))
		inptr--;
	
	match->um_eo = (inptr - in);
	
	return TRUE;
}


#ifdef BUILD_TABLE

#include <stdio.h>

/* got these from rfc1738 */
#define CHARS_LWSP " \t\n\r"               /* linear whitespace chars */
#define CHARS_SPECIAL "()<>@,;:\\\".[]"

/* got these from rfc1738 */
#define CHARS_URLSAFE "$-_.+!*'(),{}|\\^~[]`#%\";/?:@&="


static void
table_init_bits (unsigned int mask, const char *vals)
{
	int i;
	
	for (i = 0; vals[i] != '\0'; i++)
		url_scanner_table[(unsigned char) vals[i]] |= mask;
}

static void
url_scanner_table_init (void)
{
	int i;
	
	for (i = 0; i < 256; i++) {
		url_scanner_table[i] = 0;
		if (i < 32)
			url_scanner_table[i] |= IS_CTRL;
		if ((i >= '0' && i <= '9'))
			url_scanner_table[i] |= IS_DIGIT | IS_DOMAIN;
		if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z'))
			url_scanner_table[i] |= IS_ALPHA | IS_DOMAIN;
		if (i >= 128)
			url_scaller_table[i] |= IS_URLSAFE;
		if (i == 127)
			url_scanner_table[i] |= IS_CTRL;
	}
	
	url_scanner_table[' '] |= IS_SPACE;
	url_scanner_table['-'] |= IS_DOMAIN;
	
	/* not defined to be special in rfc0822, but when scanning
           backwards to find the beginning of the email address we do
           not want to include this char if we come accross it - so
           this is kind of a hack, but it's ok */
	url_scanner_table['/'] |= IS_SPECIAL;
	
	table_init_bits (IS_LWSP, CHARS_LWSP);
	table_init_bits (IS_SPECIAL, CHARS_SPECIAL);
	table_init_bits (IS_URLSAFE, CHARS_URLSAFE);
}

int main (int argc, char **argv)
{
	int i;
	
	url_scanner_table_init ();
	
	printf ("static unsigned char url_scanner_table[256] = {");
	for (i = 0; i < 256; i++) {
		printf ("%s%3d%s", (i % 16) ? "" : "\n\t",
			url_scanner_table[i], i != 255 ? "," : "\n");
	}
	printf ("};\n\n");
	
	return 0;
}

#endif /* BUILD_TABLE */
