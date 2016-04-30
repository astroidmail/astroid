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


#ifndef __G_TRIE_H__
#define __G_TRIE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GTrie GTrie;

GTrie *g_trie_new (gboolean icase);
void g_trie_free (GTrie *trie);

void g_trie_add (GTrie *trie, const char *pattern, int pattern_id);

const char *g_trie_quick_search (GTrie *trie, const char *buffer, size_t buflen, int *matched_id);

const char *g_trie_search (GTrie *trie, const char *buffer, size_t buflen, int *matched_id);

G_END_DECLS

#endif /* __G_TRIE_H__ */
