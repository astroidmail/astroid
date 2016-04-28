/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*  GMime
 *  Copyright (C) 2000-2014 Jeffrey Stedfast
 *  Copyright (C) 2016      Gaute Hope
 *
 *  Modified from gmime-filter-html.h
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


#ifndef __GMIME_FILTER_HTML_BQ_H__
#define __GMIME_FILTER_HTML_BQ_H__

#include <gmime/gmime-filter.h>

G_BEGIN_DECLS

#define GMIME_TYPE_FILTER_HTML_BQ               (g_mime_filter_html_bq_get_type ())
#define GMIME_FILTER_HTML_BQ(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMIME_TYPE_FILTER_HTML_BQ, GMimeFilterHTMLBQ))
#define GMIME_FILTER_HTML_BQ_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), GMIME_TYPE_FILTER_HTML_BQ, GMimeFilterHTMLBQClass))
#define GMIME_IS_FILTER_HTML_BQ(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMIME_TYPE_FILTER_HTML_BQ))
#define GMIME_IS_FILTER_HTML_BQ_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), GMIME_TYPE_FILTER_HTML_BQ))
#define GMIME_FILTER_HTML_BQ_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), GMIME_TYPE_FILTER_HTML_BQ, GMimeFilterHTMLBQClass))

typedef struct _GMimeFilterHTMLBQ GMimeFilterHTMLBQ;
typedef struct _GMimeFilterHTMLBQClass GMimeFilterHTMLBQClass;


/**
 * GMIME_filter_html_bq_BQ_MARK_CITATION_BLOCKQUOTE:
 *
 * Enclose citation text in blockquotes.
 **/
#define GMIME_FILTER_HTML_BQ_BLOCKQUOTE_CITATION (1 << 8)


/**
 * GMimeFilterHTMLBQ:
 * @parent_object: parent #GMimeFilter
 * @scanner: URL scanner state
 * @flags: flags specifying HTML conversion rules
 * @colour: cite colour
 * @column: current column
 * @pre_open: currently inside of a 'pre' tag.
 * @prev_cit_depth: current citation depth level.
 *
 * A filter for converting text/plain into text/html.
 **/
struct _GMimeFilterHTMLBQ {
	GMimeFilter parent_object;

	struct _UrlScanner *scanner;

	guint32 flags;
	guint32 colour;

	guint32 column       : 31;
	guint32 pre_open     : 1;
  guint32 prev_cit_depth : 31;
};

struct _GMimeFilterHTMLBQClass {
	GMimeFilterClass parent_class;

};


GType g_mime_filter_html_bq_get_type (void);

GMimeFilter *g_mime_filter_html_bq_new (guint32 flags, guint32 colour);

G_END_DECLS

#endif /* __GMIME_filter_html_bq_BQ_H__ */
