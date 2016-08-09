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


#ifndef __URL_SCANNER_H__
#define __URL_SCANNER_H__

#include <glib.h>
#include <sys/types.h>

G_BEGIN_DECLS

typedef struct {
	const char *pattern;
	const char *prefix;
	size_t um_so;
	size_t um_eo;
} urlmatch_t;

typedef gboolean (*UrlScanFunc) (const char *in, const char *pos, const char *inend, urlmatch_t *match);

/* some default GUrlScanFunc's */
G_GNUC_INTERNAL gboolean url_file_start (const char *in, const char *pos, const char *inend, urlmatch_t *match);
G_GNUC_INTERNAL gboolean url_file_end (const char *in, const char *pos, const char *inend, urlmatch_t *match);
G_GNUC_INTERNAL gboolean url_web_start (const char *in, const char *pos, const char *inend, urlmatch_t *match);
G_GNUC_INTERNAL gboolean url_web_end (const char *in, const char *pos, const char *inend, urlmatch_t *match);
G_GNUC_INTERNAL gboolean url_addrspec_start (const char *in, const char *pos, const char *inend, urlmatch_t *match);
G_GNUC_INTERNAL gboolean url_addrspec_end (const char *in, const char *pos, const char *inend, urlmatch_t *match);
G_GNUC_INTERNAL gboolean url_id_end (const char *in, const char *pos, const char *inend, urlmatch_t *match);

typedef struct {
	char *pattern;
	char *prefix;
	UrlScanFunc start;
	UrlScanFunc end;
} urlpattern_t;

typedef struct _UrlScanner UrlScanner;

G_GNUC_INTERNAL UrlScanner *url_scanner_new (void);
G_GNUC_INTERNAL void url_scanner_free (UrlScanner *scanner);

G_GNUC_INTERNAL void url_scanner_add (UrlScanner *scanner, urlpattern_t *pattern);

G_GNUC_INTERNAL gboolean url_scanner_scan (UrlScanner *scanner, const char *in, size_t inlen, urlmatch_t *match);

G_END_DECLS

#endif /* __URL_SCANNER_H__ */
