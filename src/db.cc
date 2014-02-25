# include <iostream>
# include <boost/filesystem.hpp>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  Db::Db () {
    cout << "db: opening db..";

    const char * home = getenv ("HOME");
    if (home == NULL) {
      cerr << "db: error: HOME variable not set." << endl;
      exit (1);
    }

    /* TODO: get this from config or .notmuch-config */
    path path_db (home);
    path_db /= ".mail";


    notmuch_status_t s = notmuch_database_open (path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
        &nm_db);

    if (s != 0) {
      cerr << "db: error: failed opening database." << endl;
      exit (1);
    }

    cout << "ok." << endl;

    //test_query ();
  }

  void Db::test_query () {
    cout << "db: running test query.." << endl;
    auto q = notmuch_query_create (nm_db, "label:inbox");

    cout << "query: " << notmuch_query_get_query_string (q) << ", approx: "
         << notmuch_query_count_messages (q) << " messages." << endl;

    notmuch_messages_t * messages;
    notmuch_message_t  * message;

    for (messages = notmuch_query_search_messages (q);
         notmuch_messages_valid (messages);
         notmuch_messages_move_to_next (messages)) {
      message = notmuch_messages_get (messages);

      cout << "thread:" << notmuch_message_get_thread_id (message) << ", message: " << notmuch_message_get_header (message, "Subject") << endl;

      notmuch_message_destroy (message);
    }
  }

  Db::~Db () {
    cout << "db: closing notmuch database." << endl << flush;
    if (nm_db != NULL) notmuch_database_close (nm_db);
  }

  /* --------------
   * notmuch thread
   * --------------
   */
  NotmuchThread::NotmuchThread (notmuch_thread_t * t) {
    thread_id = notmuch_thread_get_thread_id (t);
    ensure_valid ();
  }

  void NotmuchThread::ensure_valid () {
    // create new threads object
    //cout << "re-querying for thread id: " << thread_id << endl;

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

      nm_thread = notmuch_threads_get (nm_threads);

      /* update values */
      const char * s = notmuch_thread_get_subject (nm_thread);
      subject     = ustring (s);
      newest_date = notmuch_thread_get_newest_date (nm_thread);
      unread      = check_unread (nm_thread);
      attachment  = check_attachment (nm_thread);

      c++;
    }

    notmuch_threads_destroy (nm_threads);
    notmuch_query_destroy (query);
  }

  /* is there unread messages in thread */
  bool NotmuchThread::check_unread (notmuch_thread_t * nm_thread) {

    notmuch_tags_t *  tags;
    const char *      tag;

    for (tags = notmuch_thread_get_tags (nm_thread);
         notmuch_tags_valid (tags);
         notmuch_tags_move_to_next (tags))
    {
      tag = notmuch_tags_get (tags);

      if (string(tag) == "unread") {
        return true;
      }
    }

    return false;

  }

  /* is there a message with an attachment in thread */
  bool NotmuchThread::check_attachment (notmuch_thread_t * nm_thread) {

    notmuch_tags_t *  tags;
    const char *      tag;

    for (tags = notmuch_thread_get_tags (nm_thread);
         notmuch_tags_valid (tags);
         notmuch_tags_move_to_next (tags))
    {
      tag = notmuch_tags_get (tags);

      if (string(tag) == "attachment") {
        return true;
      }
    }

    return false;

  }
}

