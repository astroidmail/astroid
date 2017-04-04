# include <iostream>
# include <string>

# include <notmuch.h>
# include <gmime/gmime.h>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "db.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "utils/utils.hh"
# include "utils/date_utils.hh"
# include "utils/address.hh"
# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"
# include "actions/action_manager.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  /* --------
   * Message
   * --------
   */
  Message::Message () {
    in_notmuch = false;
    has_file   = false;
    missing_content = false;

    astroid->actions->signal_message_updated ().connect (
        sigc::mem_fun (this, &Message::on_message_updated));
  }

  Message::Message (ustring _fname) : Message () {

    LOG (info) << "msg: loading message from file: " << fname;
    fname = _fname;
    has_file   = true;
    load_message_from_file (fname);
  }

  Message::Message (ustring _mid, ustring _fname) : Message () {
    mid = _mid;
    fname = _fname;
    LOG (info) << "msg: loading message from file (mid supplied): " << fname;
    has_file   = true;
    load_message_from_file (fname);
  }

  Message::Message (notmuch_message_t *message, int _level) : Message () {
    /* The caller must make sure the message pointer
     * is valid and not destroyed while initializing */

    mid = notmuch_message_get_message_id (message);
    tid = notmuch_message_get_thread_id (message);
    in_notmuch = true;
    has_file   = true;
    level      = _level;

    nmmsg = refptr<NotmuchMessage> (new NotmuchMessage (message));

    LOG (info) << "msg: loading mid: " << mid;

    fname = nmmsg->filename;
    LOG (info) << "msg: filename: " << fname;

    load_message_from_file (fname);
    tags = nmmsg->tags;
  }

  Message::Message (refptr<NotmuchMessage> _msg) : Message () {
    in_notmuch = true;
    nmmsg = _msg;
    mid = nmmsg->mid;
    tid = nmmsg->thread_id;
    fname = nmmsg->filename;
    has_file = true;

    LOG (info) << "msg: loading mid: " << mid;
    LOG (info) << "msg: filename: " << fname;

    load_message_from_file (fname);
    tags = nmmsg->tags;
  }

  Message::Message (GMimeMessage * _msg) {
    LOG (info) << "msg: loading message from GMimeMessage.";
    in_notmuch = false;
    has_file   = false;
    missing_content = false;

    load_message (_msg);
  }

  Message::~Message () {
    LOG (debug) << "ms: deconstruct";
    if (message) g_object_unref (message);
  }

  void Message::on_message_updated (Db * db, ustring _mid) {
    if (in_notmuch && (mid == _mid)) {
      refresh (db);

      emit_message_changed (db, MessageChangedEvent::MESSAGE_TAGS_CHANGED);
    }
  }

  void Message::refresh (Db * db) {
    db->on_message (mid, [&](notmuch_message_t * msg)
      {
        if (msg != NULL) {
          in_notmuch = true;
          if (nmmsg) {
            nmmsg->refresh (msg);
          } else {
            nmmsg = refptr<NotmuchMessage> (new NotmuchMessage (msg));
          }

          fname = nmmsg->filename;
          tags  = nmmsg->tags;

        } else {
          fname = "";
          has_file = false;
          in_notmuch = false;
          missing_content = true;
        }
      });
  }

  /* message changed signal*/
  Message::type_signal_message_changed
    Message::signal_message_changed ()
  {
    return m_signal_message_changed;
  }

  void Message::emit_message_changed (Db * db, Message::MessageChangedEvent me) {
    LOG (info) << "message: emitted changed signal for message: " << mid << ": " << me;

    m_signal_message_changed.emit (db, this, me);
  }

  void Message::load_message_from_file (ustring _fname) {
    fname = _fname;
    if (!exists (fname.c_str())) {
      LOG (error) << "failed to open file: " << fname << ", it does not exist!";

      has_file = false;
      missing_content = true;

      if (in_notmuch) {

        LOG (warn) << "loading cache for missing file from notmuch";
        load_notmuch_cache ();

      } else {
        LOG (error) << "message is not in database and not on disk.";

        string error_s = "failed to open file: " + fname;
        throw message_error (error_s.c_str());
      }
      return;

    } else {
      GMimeStream   * stream  = g_mime_stream_file_new_for_path (fname.c_str(), "r");
      g_mime_stream_file_set_owner (GMIME_STREAM_FILE(stream), TRUE);
      if (stream == NULL) {
        LOG (error) << "failed to open file: " << fname << " (unspecified error)";

        has_file = false;
        missing_content = true;

        if (in_notmuch) {
          LOG (warn) << "loading cache for missing file from notmuch";
          load_notmuch_cache ();
        } else {
          LOG (error) << "tried to open disk file, but failed, message is not in database either.";
          string error_s = "failed to open file: " + fname;
          throw message_error (error_s.c_str());
        }
        return;
      }

      GMimeParser   * parser  = g_mime_parser_new_with_stream (stream);
      GMimeMessage * _message = g_mime_parser_construct_message (parser);
      load_message (_message);

      g_object_unref (_message); // is reffed in load_message
      g_object_unref (stream); // reffed from parser
      g_object_unref (parser); // reffed from message
    }
  }

  void Message::load_notmuch_cache () {
    Db db (Db::DATABASE_READ_ONLY);
    db.on_message (mid, [&](notmuch_message_t * msg)
      {

        /* read header fields */
        const char *c;
        c = notmuch_message_get_header (msg, "From");
        if (c != NULL) sender = ustring (c);
        else sender = "";

        c = notmuch_message_get_header (msg, "Subject");
        if (c != NULL) subject = ustring (c);
        else subject = "";

        c = notmuch_message_get_header (msg, "In-Reply-To");
        if (c != NULL) inreplyto = ustring (c);
        else inreplyto = "";

        c = notmuch_message_get_header (msg, "References");
        if (c != NULL) references = ustring (c);
        else references = "";

        c = notmuch_message_get_header (msg, "Reply-To");
        if (c != NULL) reply_to = ustring (c);
        else reply_to = "";

        time = notmuch_message_get_date (msg);

      });
  }

  void Message::load_message (GMimeMessage * _msg) {

    /* Load message with parts.
     *
     * example:
     * https://git.gnome.org/browse/gmime/tree/examples/basic-example.c
     *
     *
     * Do this a bit like sup:
     * - build up a tree/list of chunks that are viewable, except siblings.
     * - show text and html parts
     * - show a gallery of attachments at the bottom
     *
     */

    message = _msg;
    g_object_ref (message);
    const char *c;

    if (mid == "") {
      c = g_mime_message_get_message_id (message);
      if (c != NULL) {
        mid = ustring (c);
      } else {
        mid = UstringUtils::random_alphanumeric (10);
        mid = ustring::compose ("%1-astroid-missing-mid", mid);
        LOG (warn) << "mt: message does not have a message id, inventing one: " << mid;
      }
    }

    /* read header fields */
    c  = g_mime_message_get_sender (message);
    if (c != NULL) sender = ustring (c);
    else sender = "";

    c = g_mime_message_get_subject (message);
    if (c != NULL) subject = ustring (c);
    else subject = "";

    c = g_mime_object_get_header (GMIME_OBJECT(message), "In-Reply-To");
    if (c != NULL) inreplyto = ustring (c);
    else inreplyto = "";

    c = g_mime_object_get_header (GMIME_OBJECT(message), "References");
    if (c != NULL) references = ustring (c);
    else references = "";

    c = g_mime_object_get_header (GMIME_OBJECT(message), "Reply-To");
    if (c != NULL) reply_to = ustring (c);
    else reply_to = "";

    g_mime_message_get_date (message, &time, NULL);

    root = refptr<Chunk>(new Chunk (g_mime_message_get_mime_part (message)));
  }

  ustring Message::viewable_text (bool html, bool fallback_html) {
    /* build message body:
     * html:      output html (using gmimes html filter)
     *
     */

    if (missing_content) {
      LOG (warn) << "message: missing content, no text.";
      return "";
    }

    if (html && fallback_html) {
      throw logic_error ("message: html implies fallback_html");
    }

    ustring body;

    function< void (refptr<Chunk>) > app_body =
      [&] (refptr<Chunk> c)
    {
      /* check if we're the preferred sibling */
      bool use = false;

      if (c->siblings.size() >= 1) {
        if (c->preferred) {
          use = true;
        } else {
          /* check if there are any other preferred */
          if (all_of (c->siblings.begin (),
                      c->siblings.end (),
                      [](refptr<Chunk> c) { return (!c->preferred); })) {
            use = true;
          } else {
            use = false;
          }
        }
      } else {
        use = true;
      }

      if (use) {
        if (c->viewable && (c->preferred || html || fallback_html)) {
          body += c->viewable_text (html);
        }

        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_body);
      }
    };

    app_body (root);

    return body;
  }

  vector<refptr<Chunk>> Message::attachments () {
    /* return a flat vector of attachments */

    vector<refptr<Chunk>> attachments;

    function< void (refptr<Chunk>) > app_attachment =
      [&] (refptr<Chunk> c)
    {
      if (c->attachment)
        attachments.push_back (c);

      for_each (c->kids.begin(),
                c->kids.end (),
                app_attachment);
    };

    if (root)
      app_attachment (root);

    return attachments;
  }

  refptr<Chunk> Message::get_chunk_by_id (int id) {
    if (root->id == id) {
      return root;
    } else {
      return root->get_by_id (id);
    }
  }

  vector<refptr<Chunk>> Message::mime_messages () {
    /* return a flat vector of mime messages */

    vector<refptr<Chunk>> mime_messages;

    function< void (refptr<Chunk>) > app_mm =
      [&] (refptr<Chunk> c)
    {
      if (c->mime_message)
        mime_messages.push_back (c);

      if (!c->mime_message)
        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_mm);
    };

    if (root) app_mm (root);

    return mime_messages;
  }

  vector<refptr<Chunk>> Message::mime_messages_and_attachments () {
    /* return a flat vector of mime messages and attachments in correct order */

    vector<refptr<Chunk>> parts;

    function< void (refptr<Chunk>) > app_part =
      [&] (refptr<Chunk> c)
    {
      if (c->mime_message || c->attachment)
        parts.push_back (c);

      /* do not descend for mime messages */
      if (!c->mime_message)
        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_part);
    };

    if (root) app_part (root);

    return parts;
  }

  ustring Message::date () {
    if (missing_content) {
      ustring s;

      Db db (Db::DATABASE_READ_ONLY);
      db.on_message (mid, [&](notmuch_message_t * msg)
        {
          /* read header field */
          const char *c;

          c = notmuch_message_get_header (msg, "Date");

          if (c != NULL) s = ustring (c);
          else s = "";
        });

      return s;
    } else {
      return ustring (g_mime_message_get_date_as_string (message));
    }
  }

  ustring Message::date_asctime () {
    return Date::asctime (time);
  }

  ustring Message::pretty_date () {
    return Date::pretty_print (time);
  }

  ustring Message::pretty_verbose_date (bool include_short) {
    return Date::pretty_print_verbose (time, include_short);
  }

  InternetAddressList * Message::to () {
    if (missing_content) {
      ustring s;

      Db db (Db::DATABASE_READ_ONLY);
      db.on_message (mid, [&](notmuch_message_t * msg)
        {
          /* read header field */
          const char *c;

          c = notmuch_message_get_header (msg, "To");

          if (c != NULL) s = ustring (c);
          else s = "";
        });

      LOG (debug) << "message: cached value: " << s;
      if (s.empty ()) {
        return internet_address_list_new ();
      } else {
        return internet_address_list_parse_string (s.c_str());
      }
    } else {
      return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_TO);
    }
  }

  InternetAddressList * Message::cc () {
    if (missing_content) {
      ustring s;

      Db db (Db::DATABASE_READ_ONLY);
      db.on_message (mid, [&](notmuch_message_t * msg)
        {
          /* read header field */
          const char *c;

          c = notmuch_message_get_header (msg, "Cc");

          if (c != NULL) s = ustring (c);
          else s = "";
        });

      LOG (debug) << "message: cached value: " << s;
      if (s.empty ()) {
        return internet_address_list_new ();
      } else {
        return internet_address_list_parse_string (s.c_str());
      }
    } else {
      return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_CC);
    }
  }

  InternetAddressList * Message::bcc () {
    if (missing_content) {
      ustring s;

      Db db (Db::DATABASE_READ_ONLY);
      db.on_message (mid, [&](notmuch_message_t * msg)
        {
          /* read header field */
          const char *c;

          c = notmuch_message_get_header (msg, "Bcc");

          if (c != NULL) s = ustring (c);
          else s = "";
        });

      LOG (debug) << "message: cached value: " << s;
      if (s.empty ()) {
        return internet_address_list_new ();
      } else {
        return internet_address_list_parse_string (s.c_str());
      }
    } else {
      return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_BCC);
    }
  }

  InternetAddressList * Message::other_to () {
    InternetAddressList * ret = internet_address_list_new ();
    if (missing_content) {

      Db db (Db::DATABASE_READ_ONLY);
      db.on_message (mid, [&](notmuch_message_t * msg) {
          const char *c;
          c = notmuch_message_get_header (msg, "Delivered-To");
          if (c != NULL && strlen(c)) {
            ustring s = c;
            internet_address_list_append(ret, internet_address_list_parse_string (s.c_str()));
          }

          c = notmuch_message_get_header (msg, "Envelope-To");
          if (c != NULL && strlen(c)) {
            ustring s = c;
            internet_address_list_append(ret, internet_address_list_parse_string (s.c_str()));
          }

          c = notmuch_message_get_header (msg, "X-Original-To");
          if (c != NULL && strlen(c)) {
            ustring s = c;
            internet_address_list_append(ret, internet_address_list_parse_string (s.c_str()));
          }
          });

    } else {

      const char *c;
      c = g_mime_object_get_header (GMIME_OBJECT(message), "Delivered-To");
      if (c != NULL && strlen(c)) {
        ustring s = c;
        internet_address_list_append(ret, internet_address_list_parse_string (s.c_str()));
      }

      c = g_mime_object_get_header (GMIME_OBJECT(message), "Envelope-To");
      if (c != NULL && strlen(c)) {
        ustring s = c;
        internet_address_list_append(ret, internet_address_list_parse_string (s.c_str()));
      }

      c = g_mime_object_get_header (GMIME_OBJECT(message), "X-Original-To");
      if (c != NULL && strlen(c)) {
        ustring s = c;
        internet_address_list_append(ret, internet_address_list_parse_string (s.c_str()));
      }

    }

    return ret;
  }

  AddressList Message::list_post () {
    if (missing_content) {
      return AddressList ();
    } else {
      const char * c = g_mime_object_get_header (GMIME_OBJECT(message), "List-Post");
      if (c == NULL) return AddressList ();

      ustring _list = ustring (c);
      auto list = VectorUtils::split_and_trim (_list, " ");

      AddressList al;
      for (auto &a : list) {
        while (*a.begin () == '<') a.erase (a.begin());
        while (*(--a.end()) == '>') a.erase (--a.end ());

        ustring scheme = Glib::uri_parse_scheme (a);
        if (scheme == "mailto") {

          a = a.substr (scheme.length ()+1, a.length () - scheme.length()-1);
          UstringUtils::trim (a);
          al += Address(a);
        }
      }

      return al;
    }
  }

  AddressList Message::all_to_from () {
    return ( AddressList(to()) + AddressList(cc()) + AddressList(bcc()) + AddressList(other_to()) + Address(sender) );
  }

  ustring Message::get_filename (ustring appendix) {
    ustring _f;
    if (!missing_content) {
      _f = root->get_filename ();
    } else {
      _f = "";
    }

    if (_f.size () == 0) {
      _f = Utils::safe_fname (subject);

      if (is_patch ()) {

        if (_f.substr(0,5).uppercase() == "PATCH") _f.erase (0, 5);

        // safe_fname will catch any double _

        if (appendix.size ()) {
          _f += "-" + appendix;
        }

        _f += ".patch";

      } else {

        if (appendix.size ()) {
          _f += "-" + appendix;
        }

        _f += ".eml";
      }
    }

    _f = Utils::safe_fname (_f);

    return _f;
  }

  void Message::save () {
    if (missing_content) {
      LOG (error) << "message: missing content, can't save.";
      return;
    }

    Gtk::FileChooserDialog dialog ("Save message..",
        Gtk::FILE_CHOOSER_ACTION_SAVE);

    dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button ("_Select", Gtk::RESPONSE_OK);
    dialog.set_do_overwrite_confirmation (true);
    dialog.set_current_folder (astroid->runtime_paths ().save_dir.c_str ());

    ustring _f = get_filename ();

    dialog.set_current_name (_f);

    int result = dialog.run ();

    switch (result) {
      case (Gtk::RESPONSE_OK):
        {
          string fname = dialog.get_filename ();

          save_to (fname);

          astroid->runtime_paths ().save_dir = bfs::path (dialog.get_current_folder ());

          break;
        }

      default:
        {
          LOG (debug) << "msg: save: cancelled.";
        }
    }
  }

  void Message::save_to (ustring tofname) {
    // apparently boost needs to be compiled with -std=c++0x
    // https://svn.boost.org/trac/boost/ticket/6124
    // copy_file ( path (fname), path (tofname) );

    if (missing_content) {
      LOG (error) << "message: missing content, can't save.";
      return;
    }

    path to (tofname.c_str());
    if (is_directory (to)) {
      ustring nfname = get_filename ();
      path newto = to / path (nfname.c_str());

      while (exists (newto)) {
        nfname = get_filename (UstringUtils::random_alphanumeric (5));

        newto = to / path(nfname.c_str ());
      }

      to = newto;
    }

    tofname = ustring (to.c_str());
    LOG (info) << "msg: saving to: " << tofname;

    if (has_file)
    {
      std::ifstream src (fname, ios::binary);
      std::ofstream dst (tofname, ios::binary);

      if (!src.good () || !dst.good ()) {
        LOG (error) << "msg: failed writing to: " << tofname;
        return;
      }

      dst << src.rdbuf ();
    } else {
      /* write GMimeMessage */

      FILE * MessageFile = fopen(tofname.c_str(), "w");
      GMimeStream * stream = g_mime_stream_file_new(MessageFile);
      g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);
      g_object_unref(stream);

    }
  }

  refptr<Glib::ByteArray> Message::contents () {
    if (missing_content) {
      return Glib::ByteArray::create ();
    } else {
      return root->contents ();
    }
  }

  refptr<Glib::ByteArray> Message::raw_contents () {
    time_t t0 = clock ();

    // https://github.com/skx/lumail/blob/master/util/attachments.c

    GMimeStream * mem = g_mime_stream_mem_new ();

    g_mime_object_write_to_stream (GMIME_OBJECT(message), mem);
    g_mime_stream_flush (mem);

    GByteArray * res = g_mime_stream_mem_get_byte_array (GMIME_STREAM_MEM (mem));

    auto data = Glib::ByteArray::create ();
    if (res != NULL) {
      data->append (res->data, res->len);
    }

    g_object_unref (mem);

    LOG (info) << "message: contents: loaded " << data->size () << " bytes in " << ( (clock () - t0) * 1000.0 / CLOCKS_PER_SEC ) << " ms.";

    return data;
  }

  bool Message::is_patch () {
    return (
        (subject.substr(0,3).uppercase() != "RE:") &&
         Glib::Regex::match_simple ("\\[PATCH.*\\]", subject));
  }

  bool Message::is_different_subject () {
    return subject_is_different;
  }

  bool Message::is_list_post () {
    const char * c = g_mime_object_get_header (GMIME_OBJECT(message), "List-Post");
    return (c != NULL);
  }

  bool Message::is_encrypted () {
    return has_tag ("encrypted");
  }

  bool Message::is_signed () {
    return has_tag ("signed");
  }

  bool Message::has_tag (ustring t) {
    if (nmmsg) return nmmsg->has_tag (t);
    else return false;
  }

  /************
   * exceptions
   * **********
   */

  message_error::message_error (const char * w) : runtime_error (w)
  {
  }

  /* --------
   * MessageThread
   * --------
   */
  MessageThread::MessageThread () {
    in_notmuch = false;
  }

  MessageThread::MessageThread (refptr<NotmuchThread> _nmt) : MessageThread () {
    thread = _nmt;
    in_notmuch = true;

    astroid->actions->signal_thread_updated ().connect (
        sigc::mem_fun (this, &MessageThread::on_thread_updated));

    astroid->actions->signal_thread_changed ().connect (
        sigc::mem_fun (this, &MessageThread::on_thread_changed));
  }

  MessageThread::~MessageThread () {
    LOG (debug) << "mt: destruct.";
  }

  ustring MessageThread::get_subject () {
    return subject;
  }

  void MessageThread::set_first_subject (ustring s) {
    first_subject = s;
    first_subject = UstringUtils::replace (first_subject, "Re:", "");
    UstringUtils::trim (first_subject);

    first_subject_set = true;

    if (messages.size () == 1) {
      messages[0]->subject_is_different = true;
    }

    for (auto &m : messages) {
      m->subject_is_different = subject_is_different (m->subject);
    }
  }

  bool MessageThread::subject_is_different (ustring s) {
    s = UstringUtils::replace (s, "Re:", "");
    UstringUtils::trim (s);
    return !(s == first_subject);
  }

  void MessageThread::on_thread_updated (Db * db, ustring tid) {
    if (in_notmuch && tid == thread->thread_id) {
      thread->refresh (db);
      for (auto &m : messages) {
        m->on_message_updated (db, m->mid);
      }
    }
  }

  void MessageThread::on_thread_changed (Db * db, ustring tid) {
    if (in_notmuch && tid == thread->thread_id) {
      thread->refresh (db);
    }
  }

  bool MessageThread::has_tag (ustring t) {
    if (thread) return thread->has_tag (t);
    else return false;
  }

  void MessageThread::load_messages (Db * db) {
    /* update values */
    subject = thread->subject;
    set_first_subject (thread->subject);
    /*
    newest_date = notmuch_thread_get_newest_date (nm_thread);
    unread      = check_unread (nm_thread);
    attachment  = check_attachment (nm_thread);
    */


    /* get messages from thread */
    db->on_thread (thread->thread_id, [&](notmuch_thread_t * nm_thread)
      {

        notmuch_messages_t * qmessages;
        notmuch_message_t  * message;

        int level = 0;

        function<void(notmuch_message_t *, int)> add_replies =
          [&] (notmuch_message_t * root, int lvl) {

          notmuch_messages_t * replies;
          notmuch_message_t  * reply;

          for (replies = notmuch_message_get_replies (root);
               notmuch_messages_valid (replies);
               notmuch_messages_move_to_next (replies)) {


              reply = notmuch_messages_get (replies);
              auto m = refptr<Message>(new Message (reply, lvl));

              if (!first_subject_set) set_first_subject(m->subject);

              m->subject_is_different = subject_is_different (m->subject);
              messages.push_back (m);


              add_replies (reply, lvl + 1);

            }

          };

        for (qmessages = notmuch_thread_get_toplevel_messages (nm_thread);
             notmuch_messages_valid (qmessages);
             notmuch_messages_move_to_next (qmessages)) {

          message = notmuch_messages_get (qmessages);

          auto m = refptr<Message>(new Message (message, level));

          if (!first_subject_set) set_first_subject(m->subject);

          m->subject_is_different = subject_is_different (m->subject);
          messages.push_back (m);

          add_replies (message, level + 1);

        }

        /* check if all messages are shown: #243
         *
         * if at some point notmuch fixes this bug this code should be
         * removed for those versions of notmuch */
        if (messages.size() != (unsigned int) notmuch_thread_get_total_messages (nm_thread))
        {
          ustring mid;
          LOG (error) << "message: thread count not met! Brute force!";
          for (qmessages = notmuch_thread_get_messages (nm_thread);
               notmuch_messages_valid (qmessages);
               notmuch_messages_move_to_next (qmessages)) {
            bool found;
            found = false;

            message = notmuch_messages_get (qmessages);

            mid = notmuch_message_get_message_id (message);
            LOG (error) << "mid: " << mid;

            for (unsigned int i = 0; i < messages.size(); i ++)
            {
              if (messages[i]->mid == mid)
              {
                found = true;
                break;
              }
            }
            if ( ! found )
            {
              LOG (error) << "mid: " << mid << " was missing!";
              auto m = refptr<Message>(new Message (message, 0));

              if (!first_subject_set) set_first_subject(m->subject);

              m->subject_is_different = subject_is_different (m->subject);
              messages.push_back (m);
            }
          }

        }

      });
  }

  void MessageThread::add_message (ustring fname) {
    auto m = refptr<Message>(new Message (fname));
    if (!first_subject_set) set_first_subject(m->subject);
    m->subject_is_different = subject_is_different (m->subject);
    messages.push_back (m);
  }

  void MessageThread::add_message (refptr<Chunk> c) {
    if (!c->mime_message) {
      LOG (error) << "mt: can only add message chunks that are GMimeMessages.";
      throw runtime_error ("mt: can only add message chunks that are GMimeMessages");
    }

    messages.push_back (c->get_mime_message ());

    if (subject == "") {
      set_first_subject ((*(--messages.end()))->subject);
    }
  }

}

