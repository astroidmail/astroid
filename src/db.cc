# include <iostream>
# include <boost/filesystem.hpp>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"

using namespace std;
using namespace boost::filesystem;

namespace Gulp {
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

    test_query ();
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
  NotmuchThread::NotmuchThread () { };
  NotmuchThread::NotmuchThread (notmuch_threads_t *ts, notmuch_thread_t * t) {
    nm_thread = t;
    nm_threads = ts;
    thread_id = notmuch_thread_get_thread_id (t);
  }

  void NotmuchThread::ensure_valid () {
    // thread id is constant
    if (!notmuch_threads_valid (nm_threads)) {
      // create new threads object
      cout << "re-querying for thread id: " << thread_id << endl;

      string query_s = "threadid:" + thread_id;
      notmuch_query_t * query = notmuch_query_create (gulp->db->nm_db, query_s.c_str());

      int c = 0;

      for (nm_threads = notmuch_query_search_threads (query);
           notmuch_threads_valid (nm_threads);
           notmuch_threads_move_to_next (nm_threads)) {

        if (c > 0) {
          cerr << "notmuch_thread: got more than one thread for thread id!" << endl;
          break;
        }

        nm_thread = notmuch_threads_get (nm_threads);

        c++;
      }
    }
  }

  /* get subject of thread */
  ustring NotmuchThread::get_subject () {

    ensure_valid ();
    const char * subject = notmuch_thread_get_subject (nm_thread);

    return ustring (subject);
  }

  /* get newest date of thread */
  time_t NotmuchThread::get_newest_date () {

    ensure_valid ();

    return notmuch_thread_get_newest_date (nm_thread);
  }

  /* is there unread messages in thread */
  bool NotmuchThread::unread () {

    ensure_valid ();

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
}

