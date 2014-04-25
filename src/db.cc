# include <iostream>
# include <vector>
# include <algorithm>
# include <boost/filesystem.hpp>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  Db::Db () {

    const char * home = getenv ("HOME");
    if (home == NULL) {
      cerr << "db: error: HOME variable not set." << endl;
      exit (1);
    }

    /* TODO: get this from config or .notmuch-config */
    path path_db (home);
    path_db /= ".mail";

    cout << "db: opening db: " << path_db << endl;


    notmuch_status_t s = notmuch_database_open (path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_WRITE,
        &nm_db);

    if (s != 0) {
      cerr << "db: error: failed opening database." << endl;
      exit (1);
    }

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

  notmuch_thread_t * NotmuchThread::get_nm_thread () {

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
        return NULL;
      }

      nm_thread = notmuch_threads_get (nm_threads);

      c++;
    }

    // TODO: Free query.

    return nm_thread;
  }

  void NotmuchThread::destroy_nm_thread (notmuch_thread_t *) {

    /*
    notmuch_threads_destroy (nm_threads);
    notmuch_query_destroy (query);
    */

  }

  void NotmuchThread::ensure_valid () {
    // create new threads object
    //cout << "re-querying for thread id: " << thread_id << endl;

    notmuch_thread_t * nm_thread = get_nm_thread ();

    unread     = false;
    attachment = false;

    /* update values */
    const char * s = notmuch_thread_get_subject (nm_thread);
    subject     = ustring (s);
    newest_date = notmuch_thread_get_newest_date (nm_thread);
    total_messages = check_total_messages (nm_thread);
    authors     = get_authors (nm_thread);
    tags        = get_tags (nm_thread);


    destroy_nm_thread (nm_thread);
  }

  vector<ustring> NotmuchThread::get_tags (notmuch_thread_t * nm_thread) {

    notmuch_tags_t *  tags;
    const char *      tag;

    vector<ustring> ttags;

    for (tags = notmuch_thread_get_tags (nm_thread);
         notmuch_tags_valid (tags);
         notmuch_tags_move_to_next (tags))
    {
      tag = notmuch_tags_get (tags);

      if (string(tag) == "unread") {
        unread = true;
      } else if (string(tag) == "attachment") {
        attachment = true;
      }

      ttags.push_back (ustring(tag));
    }


    return ttags;
  }

  /* get and split authors */
  vector<ustring> NotmuchThread::get_authors (notmuch_thread_t * nm_thread) {
    ustring astr = ustring (notmuch_thread_get_authors (nm_thread));

    vector<ustring> iaths = Glib::Regex::split_simple(",", astr);
    vector<ustring> aths;

    for_each (iaths.begin (),
              iaths.end (),
              [&](ustring a) {

                /* strip */
                while (a[0] == ' ') a = a.substr(1, a.size());
                while (a[a.size()-1] == ' ') a = a.substr(0,a.size()-1);

                aths.push_back (a);
              });

    return aths;
  }

  int NotmuchThread::check_total_messages (notmuch_thread_t * nm_thread) {
    return notmuch_thread_get_total_messages (nm_thread);
  }

  /* tag actions */
  bool NotmuchThread::add_tag (ustring tag) {
    if (find(tags.begin (), tags.end (), tag) == tags.end ()) {
      notmuch_thread_t * nm_thread = get_nm_thread ();

      /* get messages from thread */
      notmuch_messages_t * qmessages;
      notmuch_message_t  * message;

      bool res = true;

      for (qmessages = notmuch_thread_get_messages (nm_thread);
           notmuch_messages_valid (qmessages);
           notmuch_messages_move_to_next (qmessages)) {

        message = notmuch_messages_get (qmessages);

        notmuch_status_t s = notmuch_message_add_tag (message, tag.c_str ());

        if (s == NOTMUCH_STATUS_SUCCESS) {
          res &= true;
        } else {
          cerr << "nm: could not add tag: " << tag << " to thread: " << thread_id << endl;
          return false;
        }

      }

      if (res) {
        tags.push_back (tag);
      }

      destroy_nm_thread (nm_thread);


      return true;
    } else {
      return false;
    }
  }

  bool NotmuchThread::remove_tag (ustring tag) {
    cout << "nm (" << thread_id << "): remove tag: " << tag << endl;
    if (find(tags.begin (), tags.end (), tag) != tags.end ()) {
      notmuch_thread_t * nm_thread = get_nm_thread ();

      /* get messages from thread */
      notmuch_messages_t * qmessages;
      notmuch_message_t  * message;

      bool res = true;

      for (qmessages = notmuch_thread_get_messages (nm_thread);
           notmuch_messages_valid (qmessages);
           notmuch_messages_move_to_next (qmessages)) {

        message = notmuch_messages_get (qmessages);

        notmuch_status_t s = notmuch_message_remove_tag (message, tag.c_str ());

        if (s == NOTMUCH_STATUS_SUCCESS) {
          res &= true;
        } else {
          cerr << "nm: could not remove tag: " << tag << " from thread: " << thread_id << endl;
          return false;
        }

      }

      if (res) {
        tags.erase (remove (tags.begin (),
                            tags.end (),
                            tag), tags.end ());
      }

      destroy_nm_thread (nm_thread);


      return true;
    } else {
      cout << "nm: thread does not have tag." << endl;
      return false;
    }
  }
}

