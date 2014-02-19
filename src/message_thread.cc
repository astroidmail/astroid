# include <iostream>
# include <string>

# include <notmuch.h>
# include <gmime/gmime.h>

# include "astroid.hh"
# include "db.hh"
# include "message_thread.hh"

using namespace std;

namespace Astroid {
  /* --------
   * Message
   * --------
   */
  Message::Message () { }
  Message::Message (ustring _fname) : fname (_fname) {
  }

  Message::Message (notmuch_message_t *message) {
    /* The initializer must make sure the message pointer
     * is valid and not destroyed while initializing */

    mid = notmuch_message_get_message_id (message);

    cout << "msg: loading mid: " << mid << endl;

    fname = notmuch_message_get_filename (message);
    cout << "msg: filename: " << fname << endl;

    load_message ();
  }

  void Message::load_message () {
    g_mime_init (0); // utf-8 is default
    GMimeStream * stream = g_mime_stream_file_new_for_path (fname.c_str(), "r");

    /* lots of work ... */

    g_object_unref (stream);
  }


  /* --------
   * MessageThread
   * --------
   */

  MessageThread::MessageThread () { }
  MessageThread::MessageThread (ustring _tid) : thread_id (_tid) {

  }

  void MessageThread::load_messages () {
    /* get thread from notmuch and load messages */

    string query_s = "thread:" + thread_id;
    notmuch_query_t * query = notmuch_query_create (astroid->db->nm_db, query_s.c_str());
    notmuch_threads_t * nm_threads;
    notmuch_thread_t  * nm_thread;

    int c = 0;

    for (nm_threads = notmuch_query_search_threads (query);
         notmuch_threads_valid (nm_threads);
         notmuch_threads_move_to_next (nm_threads)) {

      if (c > 0) {
        cerr << "notmuch_thread: got more than one thread for thread id!" << endl;
        break;
      }

      /* thread */
      nm_thread = notmuch_threads_get (nm_threads);

      c++;
    }

    if (c != 1) {
      cout << "mt: error: got more than one thread for id: " << thread_id << endl;
      return;
    }

    /* update values */
    /*
    const char * s = notmuch_thread_get_subject (nm_thread);
    subject     = ustring (s);
    newest_date = notmuch_thread_get_newest_date (nm_thread);
    unread      = check_unread (nm_thread);
    attachment  = check_attachment (nm_thread);
    */


    /* get messages from thread */
    notmuch_messages_t * qmessages;
    notmuch_message_t  * message;

    for (qmessages = notmuch_thread_get_messages (nm_thread);
         notmuch_messages_valid (qmessages);
         notmuch_messages_move_to_next (qmessages)) {

      message = notmuch_messages_get (qmessages);

      messages.push_back (refptr<Message>(new Message (message)));
    }

    notmuch_threads_destroy (nm_threads);
    notmuch_query_destroy (query);
  }

  void MessageThread::reload_messages () {
  }

}

