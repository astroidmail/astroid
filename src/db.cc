# include <iostream>
# include <vector>
# include <algorithm>
# include <exception>
# include <boost/filesystem.hpp>

# include <thread>
# include <chrono>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"
# include "utils/utils.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  Db::Db () {
    db_lock.lock ();

    config = astroid->config->config.get_child ("astroid.notmuch");

    const char * home = getenv ("HOME");
    if (home == NULL) {
      cerr << "db: error: HOME variable not set." << endl;
      exit (1);
    }

    ustring db_path = ustring (config.get<string> ("db"));

    /* replace ~ with home */
    if (db_path[0] == '~') {
      path_db = path(home) / path(db_path.substr(1,db_path.size()));
    } else {
      path_db = path(db_path);
    }

    cout << "db: opening db: " << path_db << endl;

    nm_db = NULL;
    open_db_read_only ();

    //test_query ();

    load_tags ();

    db_lock.unlock ();
  }

  void Db::close_db () {
    cout << "db: closing db." << endl;
    if (db_lock.try_lock ()) {
      if (nm_db != NULL) {
        notmuch_database_close (nm_db);
        nm_db = NULL;
      }
      db_lock.unlock ();
    } else {
      cout << "db: error: multiple locks from other threads on db." << endl;
      exit (1);
    }
  }

  bool Db::open_db_write (bool block) {
    cout << "db: open db read-write." << endl;
    if (db_lock.try_lock ()) {

      close_db ();

      notmuch_status_t s;

      int time = 0;

      do {
        s = notmuch_database_open (
          path_db.c_str(),
          notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_WRITE,
          &nm_db);

        if (s == NOTMUCH_STATUS_FILE_ERROR) {
          cout << "db: error: could not open db r/w, waiting " <<
                  db_write_open_delay << " of maximum " <<
                  db_write_open_timeout << endl;

          chrono::seconds duration (db_write_open_delay);
          this_thread::sleep_for (duration);

          time += db_write_open_delay;

        }

      } while ((s == NOTMUCH_STATUS_FILE_ERROR) && (time <= db_write_open_timeout));

      if (s != NOTMUCH_STATUS_SUCCESS) {
        cerr << "db: error: failed opening database for writing." << endl;

        exit (1); // TODO

        db_lock.unlock ();
        return false;
      }

      db_lock.unlock ();
      return true;

    } else {
      cout << "db: error: multiple locks from other threads on db." << endl;
      exit (1); return false;
    }
  }

  bool Db::open_db_read_only () {
    cout << "db: open db read-only." << endl;
    if (db_lock.try_lock ()) {

      if (nm_db != NULL) close_db ();

      notmuch_status_t s =
        notmuch_database_open (
          path_db.c_str(),
          notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
          &nm_db);

      if (s != NOTMUCH_STATUS_SUCCESS) {
        cerr << "db: error: failed opening database." << endl;

        exit (1); // TODO

        db_lock.unlock ();
        return false;
      }

      db_lock.unlock ();
      return true;

    } else {
      cout << "db: error: multiple locks from other threads on db." << endl;
      exit (1); return false;
    }
  }

  bool Db::acquire_write_lock (bool block = true) {
    bool r = false;
    if (block) {
      db_lock.lock ();
      r = true;
    } else {
      r = db_lock.try_lock ();
    }

    if (r) {
      r = open_db_write (block); // open R/W
      if (r) {
        return true;
      } else {
        open_db_read_only (); // failed opening R/W, falling back to read-only
        return false;
      }
    } else {
      // failed opening (only for try_lock)
      return false;
    }
  }

  void Db::release_write_lock () {
    open_db_read_only ();

    db_lock.unlock ();
  }

  bool Db::acquire_lock (bool block = true) {
    if (block) {
      db_lock.lock ();
      return true;
    } else {
      return db_lock.try_lock ();
    }
  }

  void Db::release_lock () {
    db_lock.unlock ();
  }


  Db::~Db () {
    cout << "db: closing notmuch database." << endl << flush;
    close_db ();
  }

  void Db::load_tags () {
    notmuch_tags_t * nm_tags = notmuch_database_get_all_tags (nm_db);
    const char * tag;

    tags.clear ();

    for (; notmuch_tags_valid (nm_tags);
           notmuch_tags_move_to_next (nm_tags))

    {
      tag = notmuch_tags_get (nm_tags);

      tags.push_back (ustring(tag));
    }

    notmuch_tags_destroy (nm_tags);

    cout << "db: loaded " << tags.size () << " tags." << endl;
  }

  void Db::add_sent_message (ustring fname) {
    cout << "db: adding sent message: " << fname << endl;

    notmuch_message_t * msg;

    notmuch_status_t s = notmuch_database_add_message (nm_db,
        fname.c_str (),
        &msg);

    if (s != NOTMUCH_STATUS_SUCCESS) {
      cout << "db: error adding message: " << s << endl;
      return;
    }

    /* add tags */
    for (ustring &t : sent_tags) {
      s = notmuch_message_add_tag (msg, t.c_str());
    }

    notmuch_message_destroy (msg);
  }

  void Db::test_query () { // {{{
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
  } // }}}


  /* --------------
   * notmuch thread
   * --------------
   */
  NotmuchThread::NotmuchThread (notmuch_thread_t * t) {
    thread_id = notmuch_thread_get_thread_id (t);
  }

  NotmuchThread::NotmuchThread (ustring tid) : thread_id (tid) { }

  NotmuchThread::~NotmuchThread () {
    if (ref > 0) {
      cerr << "nmt: notmuchthread destroyed while ref count on db object > 0" << endl;
    }
    ref = 0;
    deactivate ();
  }

  void NotmuchThread::activate () {
    ref++;

    if (ref > 1) {
      return; // already set up
    }


    string query_s = "thread:" + thread_id;
    query = notmuch_query_create (astroid->db->nm_db, query_s.c_str());

    int c = 0;
    for (nm_threads = notmuch_query_search_threads (query);
         notmuch_threads_valid (nm_threads);
         notmuch_threads_move_to_next (nm_threads)) {
      if (c > 0) {
        throw invalid_argument ("nmt: got more than one thread for thread id.");
      }

      nm_thread = notmuch_threads_get (nm_threads);

      c++;
    }
  }

  void NotmuchThread::deactivate () {
    if (--ref <= 0) {
      notmuch_threads_destroy (nm_threads);
      notmuch_query_destroy (query);
    }
  }

  void NotmuchThread::refresh () {
    /* do a new db query and update all fields */

    activate ();

    unread     = false;
    attachment = false;
    flagged    = false;

    /* update values */
    const char * s = notmuch_thread_get_subject (nm_thread);
    subject     = ustring (s);
    newest_date = notmuch_thread_get_newest_date (nm_thread);
    total_messages = check_total_messages ();
    authors     = get_authors ();
    tags        = get_tags ();


    deactivate ();
  }

  vector<ustring> NotmuchThread::get_tags () {

    activate ();
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
      } else if (string(tag) == "flagged") {
        flagged = true;
      }

      ttags.push_back (ustring(tag));
    }


    deactivate ();
    return ttags;
  }

  /* get and split authors */
  vector<ustring> NotmuchThread::get_authors () {
    activate ();

    ustring astr = ustring (notmuch_thread_get_authors (nm_thread));
    vector<ustring> aths = VectorUtils::split_and_trim (astr, ",");

    deactivate ();

    return aths;
  }

  int NotmuchThread::check_total_messages () {
    activate ();
    int c = notmuch_thread_get_total_messages (nm_thread);
    deactivate ();
    return c;
  }

  /* tag actions */
  bool NotmuchThread::add_tag (ustring tag) {
    cout << "nm (" << thread_id << "): add tag: " << tag << endl;
    tag = sanitize_tag (tag);
    if (!check_tag (tag)) {
      cout << "nm (" << thread_id << "): error, invalid tag: " << tag << endl;
      return false;
    }

    if (find(tags.begin (), tags.end (), tag) == tags.end ()) {
      activate ();

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

        // add to global tag list
        if (find(astroid->db->tags.begin (),
              astroid->db->tags.end (),
              tag) == astroid->db->tags.end ()) {
          astroid->db->tags.push_back (tag);
        }
      }

      deactivate ();


      return true;
    } else {
      return false;
    }
  }

  bool NotmuchThread::remove_tag (ustring tag) {
    cout << "nm (" << thread_id << "): remove tag: " << tag << endl;
    tag = sanitize_tag (tag);
    if (!check_tag (tag)) {
      cout << "nm (" << thread_id << "): error, invalid tag: " << tag << endl;
      return false;
    }

    if (find(tags.begin (), tags.end (), tag) != tags.end ()) {
      activate ();

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

      deactivate ();


      return true;
    } else {
      cout << "nm: thread does not have tag." << endl;
      return false;
    }
  }

  ustring NotmuchThread::sanitize_tag (ustring tag) {
    UstringUtils::trim (tag);

    return tag;
  }

  bool NotmuchThread::check_tag (ustring tag) {
    if (tag.empty()) return false;

    const vector<ustring> invalid_chars = { "\"" };

    for (const ustring &c : invalid_chars) {
      if (tag.find_first_of (c) != ustring::npos) return false;
    }

    return true;
  }
}

