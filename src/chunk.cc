# include <vector>
# include <iostream>

# include <glib.h>
# include <gmime/gmime.h>

# include "astroid.hh"
# include "chunk.hh"

namespace Astroid {
  Chunk::Chunk (GMimeObject * mp) : mime_object (mp) {
    content_type = g_mime_object_get_content_type (mime_object);

    if (GMIME_IS_PART (mime_object)) {
      viewable = true;
    } else if GMIME_IS_MESSAGE_PARTIAL (mime_object) {


    }
  }

  ustring Chunk::body () {
    if (GMIME_IS_PART(mime_object)) {
      cout << "is part" << endl;

      GMimeDataWrapper * content = g_mime_part_get_content_object (
          (GMimePart *) mime_object);


      refptr<Glib::ByteArray> bytearray = Glib::ByteArray::create ();

      GMimeStream * stream =
        g_mime_stream_mem_new_with_byte_array (bytearray->gobj());
      g_mime_stream_mem_set_owner ((GMimeStreamMem *) stream, false);


      /* convert to html */
      guint32 cite_color = 0x1e1e1e;

      /* other filters:
       *
       * GMIME_FILTER_HTML_PRE ||
       */
      guint32 html_filter_flags = GMIME_FILTER_HTML_CONVERT_NL |
                                  GMIME_FILTER_HTML_CONVERT_SPACES |
                                  GMIME_FILTER_HTML_CONVERT_URLS |
                                  GMIME_FILTER_HTML_MARK_CITATION |
                                  GMIME_FILTER_HTML_CONVERT_ADDRESSES |
                                  GMIME_FILTER_HTML_CITE;


      GMimeFilter * html_filter = g_mime_filter_html_new (
          html_filter_flags, cite_color);

      GMimeStream * filter_stream = g_mime_stream_filter_new (stream);
      g_mime_stream_filter_add ((GMimeStreamFilter *) filter_stream,
                                html_filter);
      g_mime_data_wrapper_write_to_stream (content, filter_stream);

      g_mime_stream_flush (filter_stream);

      g_object_unref (html_filter); // owned by filter_stream
      g_object_unref (filter_stream);
      g_object_unref (stream);
      g_object_unref (content);

      char * ss = (char *) bytearray->get_data ();

      return ustring(ss, bytearray->size());

    } else {

      return ustring("not part");

    }
  }

  Chunk::~Chunk () {
    g_object_unref (mime_object);
    g_object_unref (content_type);
  }
}

