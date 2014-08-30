# include <iostream>
# include <string>

# include <notmuch.h>
# include <gmime/gmime.h>

# include "astroid.hh"
# include "db.hh"
# include "log.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "utils/date_utils.hh"

using namespace std;

namespace Astroid {
  /* --------
   * Message
   * --------
   */
  Message::Message () {
    in_notmuch = false;
  }

  Message::Message (ustring _fname) : fname (_fname) {

    log << info << "msg: loading message from file: " << fname << endl;
    in_notmuch = false;
    load_message ();
  }

  Message::Message (ustring _mid, ustring _fname) {
    mid = _mid;
    fname = _fname;
    log << info << "msg: loading message from file (mid supplied): " << fname << endl;
    in_notmuch = false;
    load_message ();
  }

  Message::Message (notmuch_message_t *message) {
    /* The caller must make sure the message pointer
     * is valid and not destroyed while initializing */

    mid = notmuch_message_get_message_id (message);
    in_notmuch = true;

    log << info << "msg: loading mid: " << mid << endl;

    fname = notmuch_message_get_filename (message);
    log << info << "msg: filename: " << fname << endl;

    load_message ();
    load_tags (message);
  }

  Message::~Message () {
    // message is freed by the root chunk.
    g_object_unref (message);
  }

  void Message::load_tags (Db * db) {
    if (!in_notmuch) {
      log << error << "mt: error: message not in database." << endl;
      throw invalid_argument ("mt: load_tags on message not in database.");
    }

    if (mid == "") {
      log << error << "mt: error: mid not defined, no tags" << endl;
      throw invalid_argument ("mt: load_tags on message without message id.");
    } else {
      /* get tags from nm db */

      lock_guard<Db> grd (*db);
      db->on_message (mid, [&](notmuch_message_t * msg)
        {
          load_tags (msg);
        });
    }
  }

  void Message::load_tags (notmuch_message_t * msg) {

    tags.clear ();

    notmuch_tags_t * ntags;
    for (ntags = notmuch_message_get_tags (msg);
         notmuch_tags_valid (ntags);
         notmuch_tags_move_to_next (ntags)) {

      tags.push_back (ustring(notmuch_tags_get (ntags)));
    }

  }

  void Message::load_message () {

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


    GMimeStream   * stream  = g_mime_stream_file_new_for_path (fname.c_str(), "r");
    GMimeParser   * parser  = g_mime_parser_new_with_stream (stream);
    message = g_mime_parser_construct_message (parser);

    if (mid == "") {
      mid = g_mime_message_get_message_id (message);
    }

    /* read header fields */
    sender  = g_mime_message_get_sender (message);
    subject = g_mime_message_get_subject (message);
    const char *c;
    c = g_mime_object_get_header (GMIME_OBJECT(message), "In-Reply-To");
    if (c != NULL) inreplyto = ustring (c);

    c = g_mime_object_get_header (GMIME_OBJECT(message), "References");
    if (c != NULL) references = ustring (c);


    root = refptr<Chunk>(new Chunk (g_mime_message_get_mime_part (message)));


    g_object_unref (stream); // reffed from parser
    g_object_unref (parser); // reffed from message

    g_object_ref (message);  // TODO: a little bit at loss here -> change to
                             //       std::something.
  }

  ustring Message::viewable_text (bool html) {
    /* build message body:
     * html:      output html (using gmimes html filter)
     */
    refptr<Chunk> c = root;
    stringstream body;

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
        if (c->viewable) body << c->viewable_text(html);

        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_body);
      }
    };

    app_body (c);

    return ustring(body.str());
  }

  vector<refptr<Chunk>> Message::attachments () {
    /* return a flat vector of attachments */

    vector<refptr<Chunk>> attachments;

    refptr<Chunk> c = root;

    function< void (refptr<Chunk>) > app_attachment =
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
        if (c->attachment)
          attachments.push_back (c);

        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_attachment);
      }
    };

    app_attachment (c);

    return attachments;
  }

  ustring Message::date () {
    return ustring (g_mime_message_get_date_as_string (message));
  }

  ustring Message::pretty_verbose_date () {
    time_t t;
    g_mime_message_get_date (message, &t, NULL);

    return Date::pretty_print_verbose (t);
  }

  InternetAddressList * Message::to () {
    return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_TO);
  }

  InternetAddressList * Message::cc () {
    return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_CC);
  }

  InternetAddressList * Message::bcc () {
    return g_mime_message_get_recipients (message, GMIME_RECIPIENT_TYPE_BCC);
  }

  /* --------
   * MessageThread
   * --------
   */

  MessageThread::MessageThread () {
    in_notmuch = false;
  }

  MessageThread::MessageThread (refptr<NotmuchThread> _nmt) : thread (_nmt) {
    in_notmuch = true;

  }

  void MessageThread::load_messages (Db * db) {
    /* update values */
    subject     = thread->subject;
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

        for (qmessages = notmuch_thread_get_messages (nm_thread);
             notmuch_messages_valid (qmessages);
             notmuch_messages_move_to_next (qmessages)) {

          message = notmuch_messages_get (qmessages);

          messages.push_back (refptr<Message>(new Message (message)));
        }
      });
  }

  void MessageThread::add_message (ustring fname) {
    messages.push_back (refptr<Message>(new Message (fname)));
  }

  void MessageThread::reload_messages () {
  }

}

