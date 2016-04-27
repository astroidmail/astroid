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
#include <string.h>

#include "gtrie.h"

#ifdef ENABLE_WARNINGS
#define w(x) x
#else
#define w(x)
#endif /* ENABLE_WARNINGS */

#define d(x)

struct _trie_state {
	struct _trie_state *next;
	struct _trie_state *fail;
	struct _trie_match *match;
	unsigned int final;
	int id;
};

struct _trie_match {
	struct _trie_match *next;
	struct _trie_state *state;
	gunichar c;
};

struct _GTrie {
	struct _trie_state root;
	GPtrArray *fail_states;
	gboolean icase;
};

static void trie_match_free (struct _trie_match *match);
static void trie_state_free (struct _trie_state *state);

static struct _trie_match *
trie_match_new (void)
{
	return g_slice_new (struct _trie_match);
}

static void
trie_match_free (struct _trie_match *match)
{
	struct _trie_match *next;
	
	while (match) {
		next = match->next;
		trie_state_free (match->state);
		g_slice_free (struct _trie_match, match);
		match = next;
	}
}

static struct _trie_state *
trie_state_new (void)
{
	return g_slice_new (struct _trie_state);
}

static void
trie_state_free (struct _trie_state *state)
{
	trie_match_free (state->match);
	g_slice_free (struct _trie_state, state);
}


static inline gunichar
trie_utf8_getc (const char **in, size_t inlen)
{
	register const unsigned char *inptr = (const unsigned char *) *in;
	const unsigned char *inend = inptr + inlen;
	register unsigned char c, r;
	register gunichar m, u = 0;
	
	if (inlen == 0)
		return 0;
	
	r = *inptr++;
	if (r < 0x80) {
		*in = (const char *) inptr;
		u = r;
	} else if (r < 0xfe) { /* valid start char? */
		u = r;
		m = 0x7f80;	/* used to mask out the length bits */
		do {
			if (inptr >= inend)
				return 0;
			
			c = *inptr++;
			if ((c & 0xc0) != 0x80)
				goto error;
			
			u = (u << 6) | (c & 0x3f);
			r <<= 1;
			m <<= 5;
		} while (r & 0x40);
		
		*in = (const char *) inptr;
		
		u &= ~m;
	} else {
	error:
		*in = (*in) + 1;
		u = 0xfffe;
	}
	
	return u;
}


GTrie *
g_trie_new (gboolean icase)
{
	GTrie *trie;
	
	trie = g_new (GTrie, 1);
	trie->root.next = NULL;
	trie->root.fail = NULL;
	trie->root.match = NULL;
	trie->root.final = 0;
	
	trie->fail_states = g_ptr_array_new ();
	trie->icase = icase;
	
	return trie;
}


void
g_trie_free (GTrie *trie)
{
	g_ptr_array_free (trie->fail_states, TRUE);
	trie_match_free (trie->root.match);
	g_free (trie);
}



static struct _trie_match *
g (struct _trie_state *s, gunichar c)
{
	struct _trie_match *m = s->match;
	
	while (m && m->c != c)
		m = m->next;
	
	return m;
}

static struct _trie_state *
trie_insert (GTrie *trie, guint depth, struct _trie_state *q, gunichar c)
{
	struct _trie_match *m;
	
	m = trie_match_new ();
	m->next = q->match;
	m->c = c;
	
	q->match = m;
	q = m->state = trie_state_new ();
	q->match = NULL;
	q->fail = &trie->root;
	q->final = 0;
	q->id = 0;
	
	if (trie->fail_states->len < depth + 1) {
		unsigned int size = trie->fail_states->len;
		
		size = MAX (size + 64, depth + 1);
		g_ptr_array_set_size (trie->fail_states, size);
	}
	
	q->next = trie->fail_states->pdata[depth];
	trie->fail_states->pdata[depth] = q;
	
	return q;
}


#if d(!)0
static void 
dump_trie (struct _trie_state *s, int depth)
{
	char *p = g_alloca ((depth * 2) + 1);
	struct _trie_match *m;
	
	memset (p, ' ', depth * 2);
	p[depth * 2] = '\0';
	
	fprintf (stderr, "%s[state] %p: final=%d; pattern-id=%s; fail=%p\n",
		 p, s, s->final, s->id, s->fail);
	m = s->match;
	while (m) {
		fprintf (stderr, " %s'%c' -> %p\n", p, m->c, m->state);
		if (m->state)
			dump_trie (m->state, depth + 1);
		
		m = m->next;
	}
}
#endif


/*
 * final = empty set
 * FOR p = 1 TO #pat
 *   q = root
 *   FOR j = 1 TO m[p]
 *     IF g(q, pat[p][j]) == null
 *       insert(q, pat[p][j])
 *     ENDIF
 *     q = g(q, pat[p][j])
 *   ENDFOR
 *   final = union(final, q)
 * ENDFOR
*/

void
g_trie_add (GTrie *trie, const char *pattern, int pattern_id)
{
	const char *inptr = pattern;
	struct _trie_state *q, *q1, *r;
	struct _trie_match *m, *n;
	guint i, depth = 0;
	gunichar c;
	
	/* Step 1: add the pattern to the trie */
	
	q = &trie->root;
	
	while ((c = trie_utf8_getc (&inptr, -1))) {
		if (c == 0xfffe) {
			w(g_warning ("Invalid UTF-8 sequence in pattern '%s' at %s",
				     pattern, (inptr - 1)));
			continue;
		}
		
		if (trie->icase)
			c = g_unichar_tolower (c);
		
		m = g (q, c);
		if (m == NULL) {
			q = trie_insert (trie, depth, q, c);
		} else {
			q = m->state;
		}
		
		depth++;
	}
	
	q->final = depth;
	q->id = pattern_id;
	
	/* Step 2: compute failure graph */
	
	for (i = 0; i < trie->fail_states->len; i++) {
		q = trie->fail_states->pdata[i];
		while (q) {
			m = q->match;
			while (m) {
				c = m->c;
				q1 = m->state;
				r = q->fail;
				while (r && (n = g (r, c)) == NULL)
					r = r->fail;
				
				if (r != NULL) {
					q1->fail = n->state;
					if (q1->fail->final > q1->final)
						q1->final = q1->fail->final;
				} else {
					if ((n = g (&trie->root, c)))
						q1->fail = n->state;
					else
						q1->fail = &trie->root;
				}
				
				m = m->next;
			}
			
			q = q->next;
		}
	}
	
	d(fprintf (stderr, "\nafter adding pattern '%s' to trie %p:\n", pattern, trie));
	d(dump_trie (&trie->root, 0));
}

/*
 * Aho-Corasick
 *
 * q = root
 * FOR i = 1 TO n
 *   WHILE q != fail AND g(q, text[i]) == fail
 *     q = h(q)
 *   ENDWHILE
 *   IF q == fail
 *     q = root
 *   ELSE
 *     q = g(q, text[i])
 *   ENDIF
 *   IF isElement(q, final)
 *     RETURN TRUE
 *   ENDIF
 * ENDFOR
 * RETURN FALSE
 */

const char *
g_trie_quick_search (GTrie *trie, const char *buffer, size_t buflen, int *matched_id)
{
	const char *inptr, *inend, *prev, *pat;
	register size_t inlen = buflen;
	struct _trie_match *m = NULL;
	struct _trie_state *q;
	gunichar c;
	
	inend = buffer + buflen;
	inptr = buffer;
	
	q = &trie->root;
	pat = prev = inptr;
	while ((c = trie_utf8_getc (&inptr, inlen))) {
		inlen = (inend - inptr);
		
		if (c == 0xfffe) {
#ifdef ENABLE_WARNINGS
			prev = (inptr - 1);
			pat = buffer + buflen;
			g_warning ("Invalid UTF-8 in buffer '%.*s' at %.*s",
				   buflen, buffer, pat - prev, prev);
#endif
			
			pat = prev = inptr;
		}
		
		if (trie->icase)
			c = g_unichar_tolower (c);
		
		while (q != NULL && (m = g (q, c)) == NULL)
			q = q->fail;
		
		if (q == &trie->root)
			pat = prev;
		
		if (q == NULL) {
			q = &trie->root;
			pat = inptr;
		} else if (m != NULL) {
			q = m->state;
			
			if (q->final) {
				if (matched_id)
					*matched_id = q->id;
				
				return pat;
			}
		}
		
		prev = inptr;
	}
	
	return NULL;
}

const char *
g_trie_search (GTrie *trie, const char *buffer, size_t buflen, int *matched_id)
{
	const char *inptr, *inend, *prev, *pat;
	register size_t inlen = buflen;
	struct _trie_match *m = NULL;
	struct _trie_state *q;
	size_t matched = 0;
	gunichar c;
	
	inend = buffer + buflen;
	inptr = buffer;
	
	q = &trie->root;
	pat = prev = inptr;
	while ((c = trie_utf8_getc (&inptr, inlen))) {
		inlen = (inend - inptr);
		
		if (c == 0xfffe) {
			if (matched)
				return pat;
			
#ifdef ENABLE_WARNINGS
			prev = (inptr - 1);
			pat = buffer + buflen;
			g_warning ("Invalid UTF-8 in buffer '%.*s' at %.*s",
				   buflen, buffer, pat - prev, prev);
#endif
			
			pat = prev = inptr;
		}
		
		if (trie->icase)
			c = g_unichar_tolower (c);
		
		while (q != NULL && (m = g (q, c)) == NULL && matched == 0)
			q = q->fail;
		
		if (q == &trie->root) {
			if (matched)
				return pat;
			
			pat = prev;
		}
		
		if (q == NULL) {
			if (matched)
				return pat;
			
			q = &trie->root;
			pat = inptr;
		} else if (m != NULL) {
			q = m->state;
			
			if (q->final > matched) {
				if (matched_id)
					*matched_id = q->id;
				
				matched = q->final;
			}
		}
		
		prev = inptr;
	}
	
	return matched ? pat : NULL;
}


#ifdef TEST

static char *patterns[] = {
	"news://",
	"nntp://",
	"telnet://",
	"file://",
	"ftp://",
	"http://",
	"https://",
	"www.",
	"ftp.",
	"mailto:",
	"@"
};

static char *haystacks[] = {
	"try this url: http://www.ximian.com",
	"or, feel free to email me at fejj@ximian.com",
	"don't forget to check out www.ximian.com",
	"I've attached a file (file:///cvs/gmime/gmime/gtrie.c)",
};

int main (int argc, char **argv)
{
	const char *match;
	GTrie *trie;
	guint i;
	int id;
	
	trie = g_trie_new (TRUE);
	for (i = 0; i < G_N_ELEMENTS (patterns); i++)
		g_trie_add (trie, patterns[i], i);
	
	for (i = 0; i < G_N_ELEMENTS (haystacks); i++) {
		if ((match = g_trie_search (trie, haystacks[i], -1, &id))) {
			fprintf (stderr, "matched @ '%s' with pattern '%s'\n", match, patterns[id]);
		} else {
			fprintf (stderr, "no match\n");
		}
	}
	
	fflush (stdout);
	
	g_trie_free (trie);
	
	return match == NULL ? 0 : 1;
}
#endif /* TEST */
