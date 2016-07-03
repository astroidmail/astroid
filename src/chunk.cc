# include <vector>
# include <iostream>
# include <atomic>
# include <fstream>

# include <boost/filesystem.hpp>

# include <glib.h>
# include <gmime/gmime.h>
# include "utils/gmime/gmime-filter-html-bq.h"

# include "astroid.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "log.hh"
# include "utils/utils.hh"
# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"
# include "config.hh"
# include "crypto.hh"

namespace Astroid {

  std::atomic<uint> Chunk::nextid (0);

  Chunk::Chunk (GMimeObject * mp) : mime_object (mp) {
    using std::endl;
    id = nextid++;

    if (mp == NULL) {
      log << error << "chunk (" << id << "): got NULL mime_object." << endl;
      throw std::logic_error ("chunk: got NULL mime_object");
    }

    content_type = g_mime_object_get_content_type (mime_object);

    if (content_type) {
      log << debug << "chunk (" << id << "): content-type: " << g_mime_content_type_to_string (content_type) << endl;
    } else {
      log << warn << "chunk (" << id << "): content-type not specified, could be mime-message." << endl;
    }

    if (GMIME_IS_PART (mime_object)) {
      // has no sub-parts

      std::string disposition = g_mime_object_get_disposition(mime_object) ? : std::string();
      viewable = !(disposition == "attachment");

      const char * cid = g_mime_part_get_content_id ((GMimePart *) mime_object);
      if (cid != NULL) {
        content_id = ustring(cid);
        log << debug << "chunk: part, id: " << content_id << endl;
      }

      if (content_type != NULL) {
        if (viewable) {
          /* check if we can show this type */
          viewable = false;

          for (auto &m : viewable_types) {
            if (g_mime_content_type_is_type (content_type,
                  g_mime_content_type_get_media_type (m.second),
                  g_mime_content_type_get_media_subtype (m.second))) {

              viewable = true;
              break;
            }
          }
        }
      } else {
        viewable = false;
      }

      attachment = !viewable;

      if (g_mime_content_type_is_type (content_type,
          g_mime_content_type_get_media_type (preferred_type),
          g_mime_content_type_get_media_subtype (preferred_type)))
      {
        log << debug << "chunk: preferred." << endl;
        preferred = true;
      }

      log << debug << "chunk: is part (viewable: " << viewable << ", attachment: " << attachment << ") " << endl;

    } else if GMIME_IS_MESSAGE_PART (mime_object) {
      log << debug << "chunk: message part" << endl;

      /* contains a GMimeMessage with a potential substructure */
      GMimeMessage * msg = g_mime_message_part_get_message ((GMimeMessagePart *) mime_object);
      kids.push_back (refptr<Chunk>(new Chunk((GMimeObject *) msg)));

    } else if GMIME_IS_MESSAGE_PARTIAL (mime_object) {
      log << debug << "chunk: partial" << endl;

      GMimeMessage * msg = g_mime_message_partial_reconstruct_message (
          (GMimeMessagePartial **) &mime_object,
          g_mime_message_partial_get_total ((GMimeMessagePartial *) mime_object)
          );

      kids.push_back (refptr<Chunk>(new Chunk((GMimeObject *) msg)));


    } else if GMIME_IS_MULTIPART (mime_object) {
      log << debug << "chunk: multi part" << endl;

      int total = g_mime_multipart_get_count ((GMimeMultipart *) mime_object);

      if (GMIME_IS_MULTIPART_ENCRYPTED (mime_object) || GMIME_IS_MULTIPART_SIGNED (mime_object)) {

        if (GMIME_IS_MULTIPART_ENCRYPTED (mime_object)) {
            log << warn << "chunk: is encrypted." << endl;
            isencrypted = true;

            if (total != 2) {
              log << error << "chunk: encrypted message with not exactly 2 parts." << endl;
              return;
            }

            const char *protocol = g_mime_content_type_get_parameter (content_type, "protocol");
            Crypto cr (protocol);

            if (cr.ready) {
              GMimeObject * k = cr.decrypt_and_verify (mime_object);

              if (k != NULL) {
                auto c = refptr<Chunk>(new Chunk(k));
                c->isencrypted = true;
                c->issigned    = cr.issigned;
                kids.push_back (c);
              }
            }

        }

        if (GMIME_IS_MULTIPART_SIGNED (mime_object)) {
            log << warn << "chunk: is signed." << endl;

            /* only show first part */
            GMimeObject * mo = g_mime_multipart_get_part (
                (GMimeMultipart *) mime_object,
                0);

            auto c = refptr<Chunk>(new Chunk(mo));
            c->issigned = true;
            kids.push_back (c);

        }

      } else {

        bool alternative = (g_mime_content_type_is_type (content_type, "multipart", "alternative"));
        log << debug << "chunk: alternative: " << alternative << endl;


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
                        log << debug << "chunk: multipart: added sibling" << endl;
                        c->siblings.push_back (cc);
                      }
                    }
                  );

                if (g_mime_content_type_is_type (c->content_type,
                    g_mime_content_type_get_media_type (preferred_type),
                    g_mime_content_type_get_media_subtype (preferred_type)))
                {
                  log << debug << "chunk: multipart: preferred." << endl;
                  c->preferred = true;
                }
              }
            );
        }
      }

      log << debug << "chunk: multi part end" << endl;

    } else if GMIME_IS_MESSAGE (mime_object) {
      log << debug << "chunk: mime message" << endl;

      mime_message = true;
    }

    // TODO: check for inline PGP encryption, also its unsafe:
    //       https://dkg.fifthhorseman.net/notes/inline-pgp-harmful/
  }

  ustring Chunk::viewable_text (bool html = true) {
    using std::endl;

    GMimeStream * content_stream = NULL;

    if (GMIME_IS_PART(mime_object)) {
      log << debug << "chunk: body: part" << endl;


      if (g_mime_content_type_is_type (content_type, "text", "plain")) {
        log << debug << "chunk: plain text (out html: " << html << ")" << endl;

        GMimeDataWrapper * content = g_mime_part_get_content_object (
            (GMimePart *) mime_object);

        const char * charset = g_mime_object_get_content_type_parameter(GMIME_OBJECT(mime_object), "charset");
        GMimeStream * stream = g_mime_data_wrapper_get_stream (content);

        GMimeStream * filter_stream = g_mime_stream_filter_new (stream);

        /* convert to html */
        guint32 cite_color = 0x1e1e1e;

        /* other filters:
         *
         * GMIME_FILTER_HTML_PRE ||
         */
        guint32 html_filter_flags = GMIME_FILTER_HTML_CONVERT_NL |
                                    GMIME_FILTER_HTML_CONVERT_SPACES |
                                    GMIME_FILTER_HTML_CONVERT_URLS |
                                    GMIME_FILTER_HTML_CONVERT_ADDRESSES |
                                    GMIME_FILTER_HTML_BQ_BLOCKQUOTE_CITATION ;

        /* convert encoding */
        GMimeContentEncoding enc = g_mime_data_wrapper_get_encoding (content);
        if (enc) {
          log << debug << "enc: " << g_mime_content_encoding_to_string(enc) << endl;
        }

        GMimeFilter * filter = g_mime_filter_basic_new(enc, false);
        g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
        g_object_unref(filter);

        if (charset)
        {
          log << debug << "charset: " << charset << endl;
          if (std::string(charset) == "utf-8") {
            charset = "UTF-8";
          }

          GMimeFilter * filter = g_mime_filter_charset_new(charset, "UTF-8");
          g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
          g_object_unref(filter);
        } else {
          log << warn << "charset: not defined." << endl;
        }

        if (html) {

          GMimeFilter * html_filter;
          html_filter = g_mime_filter_html_bq_new (html_filter_flags, cite_color);
          g_mime_stream_filter_add (GMIME_STREAM_FILTER(filter_stream),
                                  html_filter);
          g_object_unref (html_filter);

        } else {

          /* CRLF to LF */
          GMimeFilter * crlf_filter = g_mime_filter_crlf_new (false, false);
          g_mime_stream_filter_add (GMIME_STREAM_FILTER (filter_stream),
              crlf_filter);
          g_object_unref (crlf_filter);

        }

        g_mime_stream_reset (stream);

        content_stream = filter_stream;

      } else if (g_mime_content_type_is_type (content_type, "text", "html")) {
        log << debug << "chunk: html text" << endl;

        GMimeDataWrapper * content = g_mime_part_get_content_object (
            (GMimePart *) mime_object);

        const char * charset = g_mime_object_get_content_type_parameter(GMIME_OBJECT(mime_object), "charset");
        GMimeStream * stream = g_mime_data_wrapper_get_stream (content);

        GMimeStream * filter_stream = g_mime_stream_filter_new (stream);

        /* convert encoding */
        GMimeContentEncoding enc = g_mime_data_wrapper_get_encoding (content);
        if (enc) {
          log << debug << "enc: " << g_mime_content_encoding_to_string(enc) << endl;
        }

        GMimeFilter * filter = g_mime_filter_basic_new(enc, false);
        g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
        g_object_unref(filter);

        if (charset)
        {
          log << debug << "charset: " << charset << endl;
          if (std::string(charset) == "utf-8") {
            charset = "UTF-8";
          }

          GMimeFilter * filter = g_mime_filter_charset_new(charset, "UTF-8");
          g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), filter);
          g_object_unref(filter);
        } else {
          log << warn << "charset: not defined" << endl;
        }



        g_mime_stream_reset (stream);

        content_stream = filter_stream;
      }
    }

    if (content_stream != NULL) {
      char buffer[4097];
      ssize_t prevn = 1;
      ssize_t n;
      std::stringstream sstr;

      while ((n = g_mime_stream_read (content_stream, buffer, 4096), n) >= 0)
      {
        buffer[n] = 0;
        sstr << buffer;

        if (n == 0 && prevn == 0) {
          break;
        }

        prevn = n;
      }

      g_object_unref (content_stream);

      ustring b;
      try {
        b = sstr.str();
      } catch (Glib::ConvertError &ex) {
        log << error << "could not convert chunk to utf-8, contents: " << sstr.str() << endl;
        throw ex;
      }

      return b;
    } else {
      return ustring ("Error: Non-viewable part!");
      log << error << "chunk: tried to display non-viewable part." << endl;
      //throw runtime_error ("chunk: tried to display non-viewable part.");
    }
  }

  ustring Chunk::get_filename () {
    if (_fname.size () > 0) {
      return _fname;
    }

    if (GMIME_IS_PART (mime_object)) {
      const char * s = g_mime_part_get_filename (GMIME_PART(mime_object));

      if (s != NULL) {
        ustring fname (s);
        _fname = fname;
        return fname;
      }
    } else if (GMIME_IS_MESSAGE (mime_object)) {
      const char * s = g_mime_message_get_subject (GMIME_MESSAGE (mime_object));

      if (s != NULL) {
        ustring fname (s);
        _fname = fname + ".eml";
        return fname;
      }
    }
    // no filename specified
    return ustring ("");
  }

  size_t Chunk::get_file_size () {
    time_t t0 = clock ();

    // https://github.com/skx/lumail/blob/master/util/attachments.c

    refptr<Glib::ByteArray> cnt = contents ();
    size_t sz = cnt->size ();

    log << info << "chunk: file size: " << sz << " (time used to calculate: " << ( (clock () - t0) * 1000.0 / CLOCKS_PER_SEC ) << " ms.)" << std::endl;

    return sz;
  }

  refptr<Glib::ByteArray> Chunk::contents () {
    time_t t0 = clock ();

    // https://github.com/skx/lumail/blob/master/util/attachments.c

    GMimeStream * mem = g_mime_stream_mem_new ();

    if (GMIME_IS_PART (mime_object)) {

      GMimeDataWrapper * content = g_mime_part_get_content_object (GMIME_PART (mime_object));

      g_mime_data_wrapper_write_to_stream (content, mem);

    } else {

      g_mime_object_write_to_stream (mime_object, mem);
      g_mime_stream_flush (mem);

    }

    GByteArray * res = g_mime_stream_mem_get_byte_array (GMIME_STREAM_MEM (mem));

    auto data = Glib::ByteArray::create ();
    if (res != NULL) {
      data->append (res->data, res->len);
    }

    g_object_unref (mem);

    log << info << "chunk: contents: loaded " << data->size () << " bytes in " << ( (clock () - t0) * 1000.0 / CLOCKS_PER_SEC ) << " ms." << std::endl;

    return data;
  }

  bool Chunk::save_to (std::string filename, bool overwrite) {
    /* saves chunk to file name, if filename is dir, own name */
    using std::endl;
    using bfs::path;

    path to (filename.c_str());

    if (is_directory (to)) {
      ustring fname = Utils::safe_fname (get_filename ());

      if (fname.size () == 0) {
        if (content_id != "") {
          fname = ustring::compose ("astroid-attachment-%1", content_id);
        } else {
          /* make up a name */
          path new_to;

          do {
            fname = ustring::compose ("astroid-attachment-%1", UstringUtils::random_alphanumeric (5));

            new_to = to / path(fname.c_str ());
          } while (exists (new_to));
        }
      }

      to /= path (fname.c_str ());
    }

    log << info << "chunk: saving to: " << to << endl;

    if (exists (to)) {
      if (!overwrite) {
        log << error << "chunk: save: file already exists! not writing." << endl;
        return false;
      } else {
        log << warn << "chunk: save: file already exists: overwriting." << endl;
      }
    }

    if (!exists(to.parent_path ()) || !is_directory (to.parent_path())) {
      log << error << "chunk: save: parent path does not exist or is not a directory." << endl;
      return false;
    }

    std::ofstream f (to.c_str (), std::ofstream::binary);

    auto data = contents ();
    f.write (reinterpret_cast<char*>(data->get_data ()), data->size ());

    f.close ();

    return true;
  }

  refptr<Chunk> Chunk::get_by_id (int _id, bool check_siblings) {
    if (check_siblings) {
      for (auto c : siblings) {
        if (c->id == _id) {
          return c;
        } else {
          auto kc = c->get_by_id (_id, false);
          if (kc) return kc;
        }
      }
    }

    for (auto c : kids) {
      if (c->id == _id) {
        return c;
      } else {
        auto kc = c->get_by_id (_id, true);
        if (kc) return kc;
      }
    }

    return refptr<Chunk>();
  }

  void Chunk::open () {
    using bfs::path;
    log << info << "chunk: " << get_filename () << ", opening.." << std::endl;

    path tf = astroid->standard_paths().cache_dir;

    ustring tmp_fname = ustring::compose("%1-%2", UstringUtils::random_alphanumeric (10), Utils::safe_fname(get_filename ()));
    tf /= path (tmp_fname.c_str());

    log << debug << "chunk: saving to tmp path: " << tf.c_str() << std::endl;
    save_to (tf.c_str());

    ustring tf_p (tf.c_str());

    Glib::Threads::Thread::create (
        sigc::bind (
          sigc::mem_fun (this, &Chunk::do_open),
          tf_p ));
  }

  void Chunk::do_open (ustring tf) {
    using std::endl;
    ustring external_cmd = astroid->config().get<std::string> ("attachment.external_open_cmd");

    std::vector<std::string> args = { external_cmd.c_str(), tf.c_str () };
    log << debug << "chunk: spawning: " << args[0] << ", " << args[1] << endl;
    std::string stdout;
    std::string stderr;
    int    exitcode;
    try {
      Glib::spawn_sync ("",
                        args,
                        Glib::SPAWN_DEFAULT | Glib::SPAWN_SEARCH_PATH,
                        sigc::slot <void> (),
                        &stdout,
                        &stderr,
                        &exitcode
                        );

    } catch (Glib::SpawnError &ex) {
      log << error << "chunk: exception while opening attachment: " <<  ex.what () << endl;
      log << info << "chunk: deleting tmp file: " << tf << endl;
      unlink (tf.c_str());
    }

    ustring ustdout = ustring(stdout);
    for (ustring &l : VectorUtils::split_and_trim (ustdout, ustring("\n"))) {

      log << debug << l << endl;
    }

    ustring ustderr = ustring(stderr);
    for (ustring &l : VectorUtils::split_and_trim (ustderr, ustring("\n"))) {

      log << debug << l << endl;
    }

    if (exitcode != 0) {
      log << error << "chunk: chunk script exited with code: " << exitcode << endl;
    }

    log << info << "chunk: deleting tmp file: " << tf << endl;
    unlink (tf.c_str());
  }

  bool Chunk::any_kids_viewable () {
    if (viewable) return true;

    for (auto &k : kids) {
      if (k->any_kids_viewable ()) return true;
    }

    return false;
  }

  bool Chunk::any_kids_viewable_and_preferred () {
    if (viewable && preferred) return true;

    for (auto &k : kids) {
      if (k->any_kids_viewable_and_preferred ()) return true;
    }

    return false;
  }

  ustring Chunk::get_content_type () {
    return ustring (g_mime_content_type_to_string (content_type));
  }

  void Chunk::save () {
    using std::endl;
    log << info << "chunk: " << get_filename () << ", saving.." << endl;
    Gtk::FileChooserDialog dialog ("Save attachment to folder..",
        Gtk::FILE_CHOOSER_ACTION_SAVE);

    dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button ("_Select", Gtk::RESPONSE_OK);

    dialog.set_do_overwrite_confirmation (true);
    dialog.set_current_name (Utils::safe_fname (get_filename ()));

    int result = dialog.run ();

    switch (result) {
      case (Gtk::RESPONSE_OK):
        {
          std::string fname = dialog.get_filename ();
          log << info << "chunk: saving attachment to: " << fname << endl;

          /* the dialog asks whether to overwrite or not */
          save_to (fname, true);

          break;
        }

      default:
        {
          log << debug << "chunk: save: cancelled." << endl;
        }
    }
  }

  refptr<Message> Chunk::get_mime_message () {
    if (!mime_message) {
      log << error << "chunk: this is not a mime message." << std::endl;
      throw std::runtime_error ("chunk: not a mime message");
    }

    refptr<Message> m = refptr<Message> ( new Message (GMIME_MESSAGE(mime_object)) );

    return m;
  }

  Chunk::~Chunk () {
    // these should not be unreffed.
    // g_object_unref (mime_object);
    // g_object_unref (content_type);
  }
}

