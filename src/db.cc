# include <iostream>
# include <vector>
# include <algorithm>
# include <exception>
# include <boost/filesystem.hpp>

# include <thread>
# include <chrono>
# include <atomic>
# include <condition_variable>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"
# include "utils/utils.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  Db::Db () {
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

    writers = 0;
    readers = 0;

    db_state  = CLOSED;
    nm_db     = NULL;
    open_db_read_only ();

    //test_query ();

    load_tags ();
  }

  void Db::reopen () {
    cout << "db: reopening datbase.." << endl;

    /* wait for writers or readers */
    unique_lock<mutex> wlk (writers_mux);
    writers_cv.wait (wlk, [&] { return (writers == 0); });

    unique_lock<mutex> rlk (readers_mux);
    readers_cv.wait (rlk, [&] { return (readers == 0); });

    db_ready_mut.lock ();

    /* we now have all locks */
    bool reopen_rw = (db_state == READ_WRITE);

    /* these methods close the db before opening */
    if (reopen_rw) {
      open_db_write (true);
    } else {
      open_db_read_only ();
    }

    /* release locks and let waiters know */
    db_ready_mut.unlock ();
    wlk.unlock ();
    rlk.unlock ();

    writers_cv.notify_all ();
    readers_cv.notify_all ();
  }

  void Db::close_db () {
    if (nm_db != NULL) {
      cout << "db: closing db." << endl;
      notmuch_database_close (nm_db);
      nm_db = NULL;
      db_state = CLOSED;
    } else {
      cout << "db: already closed." << endl;
    }
  }

  bool Db::open_db_write (bool block) {
    cout << "db: open db read-write." << endl;
    close_db ();
    db_state = IN_CHANGE;

    notmuch_status_t s;

    int time = 0;

    /* in case a long notmuch new or similar operation is running
     * we won't be able to get read-write access to the db untill
     * it is done.
     */
    do {
      s = notmuch_database_open (
        path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_WRITE,
        &nm_db);

      if (s == NOTMUCH_STATUS_XAPIAN_EXCEPTION) {
        cout << "db: error: could not open db r/w, waited " <<
                time << " of maximum " <<
                db_write_open_timeout << " seconds." << endl;

        chrono::seconds duration (db_write_open_delay);
        this_thread::sleep_for (duration);

        time += db_write_open_delay;

      }

    } while ((s == NOTMUCH_STATUS_XAPIAN_EXCEPTION) && (time <= db_write_open_timeout));

    if (s != NOTMUCH_STATUS_SUCCESS) {
      cerr << "db: error: failed opening database for writing." << endl;

      exit (1); // TODO

      db_state = CLOSED;

      return false;
    }

    db_state = READ_WRITE;
    cout << "db: open in r/w mode." << endl;
    return true;
  }

  bool Db::open_db_read_only () {
    cout << "db: open db read-only." << endl;
    close_db ();

    db_state = IN_CHANGE;

    notmuch_status_t s =
      notmuch_database_open (
        path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
        &nm_db);

    if (s != NOTMUCH_STATUS_SUCCESS) {
      cerr << "db: error: failed opening database." << endl;

      exit (1); // TODO

      db_state = CLOSED;

      return false;
    }

    db_state = READ_ONLY;
    cout << "db: open in read-only mode." << endl;
    return true;
  }

  void Db::write_lock () {
    cout << "db: acquire write lock.." << endl;
    writers_mux.lock ();

    if (writers == 0) {
      cout << "db: no active writers, must open db for r/w.." << endl;

      lock_guard<mutex> r_lk (db_ready_mut);
      db_state = IN_CHANGE; // no new readers may be added

      /* wait for all readers to finish */
      cout << "db: waiting for readers: " << readers << endl;
      unique_lock<mutex> lk (readers_mux);
      readers_cv.wait (lk, [&] { return (readers == 0); });

      cout << "db: no readers left, changing to r/w.." << endl;

      open_db_write (true);
      db_ready_cv.notify_all (); // let all readers know the db is ready again.

    } else {
      cout << "db: already open as r/w." << endl;

    }

    writers++;
    writers_mux.unlock ();
  }

  void Db::write_release () {
    cout << "db: writing done. " << endl;
    writers_mux.lock ();

    if (writers == 1) {
      /* we're last, close db to read-only */
      cout << "db: all writers done, going to read-only." << endl;

      lock_guard<mutex> r_lk (db_ready_mut);
      db_state = IN_CHANGE; // no new readers may be added

      /* wait for all readers to finish */
      cout << "db: waiting for readers: " << readers << endl;
      unique_lock<mutex> lk (readers_mux);
      readers_cv.wait (lk, [&] { return (readers == 0); });

      cout << "db: no readers left, changing to read-only.." << endl;

      open_db_read_only ();
      db_ready_cv.notify_all (); // let all readers know the db is ready again.

    } else {
      cout << "db: other writers left, leaving db in r/w." << endl;
    }

    writers--;

    writers_mux.unlock ();
    writers_cv.notify_all ();
  }

  void Db::read_lock () {

    unique_lock<mutex> lk (db_ready_mut);

    db_ready_cv.wait (lk,
        [&] {
          return ((db_state == READ_ONLY) || (db_state == READ_WRITE));
        });

    lock_guard <mutex> r_lk (readers_mux);

    readers++;
  }

  void Db::read_release () {
    lock_guard<mutex> lk (readers_mux);
    readers--;

    readers_cv.notify_all ();
  }


  Db::~Db () {
    cout << "db: closing notmuch database." << endl << flush;

    /* wait for writers or readers */
    unique_lock<mutex> wlk (writers_mux);
    writers_cv.wait (wlk, [&] { return (writers == 0); });
    unique_lock<mutex> rlk (readers_mux);
    readers_cv.wait (rlk, [&] { return (readers == 0); });

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
    write_lock ();

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
    write_release ();
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
    db = astroid->db;
  }

  NotmuchThread::NotmuchThread (ustring tid) : thread_id (tid) {
    db = astroid->db;
  }

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

    db->read_lock ();
    query = notmuch_query_create (astroid->db->nm_db, query_s.c_str());
    nm_threads = notmuch_query_search_threads (query);

    if (nm_threads == NULL) {
      /* try to reopen db in case we got an Xapian::DatabaseModifiedError */
      cout << "nmt: activate: trying to reopen database.." << endl;

      db->read_release ();
      astroid->db->reopen ();

      db->read_lock ();
      query = notmuch_query_create (astroid->db->nm_db, query_s.c_str());
      nm_threads = notmuch_query_search_threads (query);

      if (nm_threads == NULL) {
        /* could not do that, failing */
        cout << "nmt: to no avail, failure is iminent." << endl;

        throw database_error ("nmt: could not get a valid notmuch_threads_t from query.");

      }
    }

    int c = 0;
    for ( ; notmuch_threads_valid (nm_threads);
         notmuch_threads_move_to_next (nm_threads)) {
      if (c > 0) {
        throw invalid_argument ("nmt: got more than one thread for thread id.");
      }

      nm_thread = notmuch_threads_get (nm_threads);

      c++;
    }

    if (c < 1) {
      throw invalid_argument ("nmt: could not find thread!");
    }

    db->read_release ();
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

    db->read_lock ();
    /* update values */
    const char * s = notmuch_thread_get_subject (nm_thread);
    subject     = ustring (s);
    newest_date = notmuch_thread_get_newest_date (nm_thread);
    total_messages = check_total_messages ();
    authors     = get_authors ();
    tags        = get_tags ();
    db->read_release ();


    deactivate ();
  }

  vector<ustring> NotmuchThread::get_tags () {

    activate ();
    notmuch_tags_t *  tags;
    const char *      tag;

    vector<ustring> ttags;

    db->read_lock ();
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
    db->read_release ();


    deactivate ();
    return ttags;
  }

  /* get and split authors */
  vector<ustring> NotmuchThread::get_authors () {
    activate ();

    db->read_lock ();
    ustring astr = ustring (notmuch_thread_get_authors (nm_thread));
    db->read_release ();

    vector<ustring> aths = VectorUtils::split_and_trim (astr, ",");

    deactivate ();

    return aths;
  }

  int NotmuchThread::check_total_messages () {
    activate ();
    db->read_lock ();
    int c = notmuch_thread_get_total_messages (nm_thread);
    db->read_release ();
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
      db->write_lock ();

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
          db->write_release ();
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

      db->write_release ();
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
      db->write_lock ();

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
          db->write_release ();
          return false;
        }

      }

      if (res) {
        tags.erase (remove (tags.begin (),
                            tags.end (),
                            tag), tags.end ());
      }

      db->write_release ();
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

    if (tag.size() > NOTMUCH_TAG_MAX) {
      cout << "nmt: error: maximum tag length is: " << NOTMUCH_TAG_MAX << endl;
      return false;
    }

    return true;
  }

  /************
   * exceptions
   * **********
   */

  database_error::database_error (const char * w) : runtime_error (w)
  {
  }


}

