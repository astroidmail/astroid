# include <vector>
# include <iostream>

# include <glib.h>
# include <gmime/gmime.h>

# include "astroid.hh"
# include "chunk.hh"

namespace Astroid {
  Chunk::Chunk (GMimeObject * mp) : mime_object (mp) {
    content_type = g_mime_object_get_content_type (mime_object);

    cout << "chunk: content-type: " << g_mime_content_type_to_string (content_type) << endl;

    if (GMIME_IS_PART (mime_object)) {
      // has no sub-parts

      string disposition = g_mime_object_get_disposition(mime_object) ? : string();
      viewable = !(disposition == "attachment");

      const char * cid = g_mime_part_get_content_id ((GMimePart *) mime_object);
      if (cid != NULL) {
        content_id = ustring(cid);
      }

      cout << "chunk: part, id: " << content_id << endl;

    } else if GMIME_IS_MESSAGE_PART (mime_object) {
      cout << "chunk: message part" << endl;

      /* contains a GMimeMessage with a potential substructure */
      GMimeMessage * msg = g_mime_message_part_get_message ((GMimeMessagePart *) mime_object);
      kids.push_back (refptr<Chunk>(new Chunk((GMimeObject *) msg)));

    } else if GMIME_IS_MESSAGE_PARTIAL (mime_object) {
      cout << "chunk: partial" << endl;

      GMimeMessage * msg = g_mime_message_partial_reconstruct_message (
          (GMimeMessagePartial **) &mime_object,
          g_mime_message_partial_get_total ((GMimeMessagePartial *) mime_object)
          );

      kids.push_back (refptr<Chunk>(new Chunk((GMimeObject *) msg)));

    } else if GMIME_IS_MULTIPART (mime_object) {
      cout << "chunk: multi part" << endl;
      //  TODO: MultiPartEncrypted, MultiPartSigned

      bool alternative = (g_mime_content_type_is_type (content_type, "multipart", "alternative"));
      cout << "chunk: alternative: " << alternative << endl;

      int total = g_mime_multipart_get_count ((GMimeMultipart *) mime_object);

      for (int i = 0; i < total; i++) {
        GMimeObject * mo = g_mime_multipart_get_part (
            (GMimeMultipart *) mime_object,
            i);

        kids.push_back (refptr<Chunk>(new Chunk(mo)));
      }

      if (alternative) {
        for_each (
            kids.begin(),
            kids.end(),
            [&] (refptr<Chunk> c) {
              for_each (
                  kids.begin(),
                  kids.end(),
                  [&] (refptr<Chunk> cc) {
                    if (c != cc) {
                      cout << "chunk: multipart: added sibling" << endl;
                      c->siblings.push_back (cc);
                    }
                  }
                );

              if (g_mime_content_type_is_type (c->content_type,
                  g_mime_content_type_get_media_type (preferred_type),
                  g_mime_content_type_get_media_subtype (preferred_type)))
              {
                cout << "chunk: multipart: preferred." << endl;
                c->preferred = true;
              }
            }
          );
      }

      cout << "chunk: multi part end" << endl;

    } else if GMIME_IS_MESSAGE (mime_object) {
      cout << "chunk: mime message" << endl;

    }
  }

  ustring Chunk::body (bool html = true) {
    if (GMIME_IS_PART(mime_object)) {
      cout << "chunk: body: part" << endl;
      if (g_mime_content_type_is_type (content_type, "text", "plain")) {
        cout << "chunk: plain text" << endl;

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

        GMimeFilter * html_filter;
        if (html)
          html_filter = g_mime_filter_html_new (html_filter_flags, cite_color);

        GMimeStream * filter_stream = g_mime_stream_filter_new (stream);

        if (html)
          g_mime_stream_filter_add ((GMimeStreamFilter *) filter_stream,
                                  html_filter);

        GMimeFilter * filter = g_mime_filter_basic_new(g_mime_data_wrapper_get_encoding(content), false);
        g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
        g_object_unref(filter);

        const char * charset = g_mime_object_get_content_type_parameter(GMIME_OBJECT(mime_object), "charset");
        if (charset)
        {
            GMimeFilter * filter = g_mime_filter_charset_new(charset, "UTF-8");
            g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
            g_object_unref(filter);
        }

        g_mime_data_wrapper_write_to_stream (content, filter_stream);

        g_mime_stream_flush (filter_stream);

        if (html)
          g_object_unref (html_filter); // owned by filter_stream
        g_object_unref (filter_stream);
        g_object_unref (stream);
        g_object_unref (content);

        const char * ss = (const char *) bytearray->get_data ();

        viewable = true;

        return ustring(ss); // also issues with size in plain-text mode

      } else if (g_mime_content_type_is_type (content_type, "text", "html")) {
        cout << "chunk: html text" << endl;

        GMimeDataWrapper * content = g_mime_part_get_content_object (
            (GMimePart *) mime_object);


        refptr<Glib::ByteArray> bytearray = Glib::ByteArray::create ();

        GMimeStream * stream =
          g_mime_stream_mem_new_with_byte_array (bytearray->gobj());

        g_mime_stream_mem_set_owner ((GMimeStreamMem *) stream, false);

        g_mime_data_wrapper_write_to_stream (content, stream);
        g_mime_stream_flush (stream);

        g_object_unref (stream);
        g_object_unref (content);

        const char * ss = (const char *) bytearray->get_data ();

        viewable = true;

        return ustring (ss); // TODO: apparently some issues with using the
                             //       bytearray size here..

      } else {
        viewable = false;
        return ustring ("non-viewable part");
      }
    } else {

      viewable = false;
      return ustring("not part");

    }
  }

  Chunk::~Chunk () {
    //g_object_unref (mime_object); // TODO: not sure about this one..
    g_object_unref (content_type);
  }
}

