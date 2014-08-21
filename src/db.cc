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
  Db::Db (DbMode mode) {
    config = astroid->config->config.get_child ("astroid.notmuch");

    const char * home = getenv ("HOME");
    if (home == NULL) {
      cerr << "db: error: HOME variable not set." << endl;
      throw invalid_argument ("db: error: HOME environment variable not set.");
    }

    ustring db_path = ustring (config.get<string> ("db"));

    /* replace ~ with home */
    if (db_path[0] == '~') {
      path_db = path(home) / path(db_path.substr(1,db_path.size()));
    } else {
      path_db = path(db_path);
    }

    cout << "db: opening db: " << path_db << endl;

    time_t start = clock ();

    db_state  = CLOSED;
    nm_db     = NULL;
    if (mode == DATABASE_READ_ONLY) {
      open_db_read_only ();
    } else if (mode == DATABASE_READ_WRITE) {
      open_db_write (true);
    } else {
      throw invalid_argument ("db: mode must be read-only or read-write");
    }

    float diff = (clock () - start) * 1000.0 / CLOCKS_PER_SEC;
    cout << "db: open time: " << diff << " ms." << endl;
  }

  void Db::reopen () {
    cout << "db: reopening datbase.." << endl;

    /* we now have all locks */
    bool reopen_rw = (db_state == READ_WRITE);

    /* these methods close the db before opening */
    if (reopen_rw) {
      open_db_write (true);
    } else {
      open_db_read_only ();
    }
  }

  bool Db::check_reopen (bool doreopen) {
    /* tries to provoke an Xapian::DatabaseModifiedError and
     * optionally reopens the database.
     *
     * returns true if db is invalid. queries will be invalid
     * afterwards, but might still respond true to notmuch_valid
     * calls.
     *
     */

    time_t start = clock ();
    bool invalid = false;

    /* testing */
    notmuch_query_t * q = notmuch_query_create (nm_db, "thread:fail");

    /* this will produce an error, but no way to ensure failure */
    notmuch_query_count_threads (q);

    /* this returns NULL when error */
    notmuch_threads_t * threads = notmuch_query_search_threads (q);

    if (threads == NULL) invalid = true;

    notmuch_query_destroy (q);

    float diff = (clock () - start) * 1000.0 / CLOCKS_PER_SEC;
    cout << "db: test query time: " << diff << " ms." << endl;

    if (invalid) {
      cout << "db: no longer valid, reopen required." << endl;
    } else {
      cout << "db: valid." << endl;
    }

    if (invalid && doreopen) {
      reopen ();
    }


    return invalid;
  }

  void Db::close_db () {
    if (nm_db != NULL) {
      cout << "db: closing db." << endl;
      notmuch_database_close (nm_db);
      nm_db = NULL;
      db_state = CLOSED;
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

      throw database_error ("failed to open database for writing");

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

      throw database_error ("failed to open database (read-only)");

      db_state = CLOSED;

      return false;
    }

    db_state = READ_ONLY;
    cout << "db: open in read-only mode." << endl;
    return true;
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

  void Db::on_thread (ustring thread_id, function<void(notmuch_thread_t *)> func) {

    string query_s = "thread:" + thread_id;

    notmuch_query_t * query = notmuch_query_create (nm_db, query_s.c_str());
    notmuch_threads_t * nm_threads = notmuch_query_search_threads (query);
    notmuch_thread_t  * nm_thread;

    if (nm_threads == NULL) {
      /* try to reopen db in case we got an Xapian::DatabaseModifiedError */
      cout << "db: trying to reopen database.." << endl;

      reopen ();

      query = notmuch_query_create (nm_db, query_s.c_str());
      nm_threads = notmuch_query_search_threads (query);

      if (nm_threads == NULL) {
        /* could not do that, failing */
        cout << "db: to no avail, failure is iminent." << endl;

        throw database_error ("nmt: could not get a valid notmuch_threads_t from query.");

      }
    }

    int c = 0;
    for ( ; notmuch_threads_valid (nm_threads);
         notmuch_threads_move_to_next (nm_threads)) {
      if (c > 0) {
        throw invalid_argument ("db: got more than one thread for thread id.");
      }

      nm_thread = notmuch_threads_get (nm_threads);

      c++;
    }

    if (c < 1) {
      throw invalid_argument ("db: could not find thread!");
    }

    /* call function */
    func (nm_thread);

    /* free resources */
    notmuch_query_destroy (query);
  }

  void Db::on_message (ustring mid, function<void(notmuch_message_t *)> func) {

    notmuch_message_t * msg;
    notmuch_status_t s = notmuch_database_find_message (nm_db, mid.c_str(), &msg);

    if (s != NOTMUCH_STATUS_SUCCESS) {
      /* try to reopen db in case we got an Xapian::DatabaseModifiedError */
      cout << "db: trying to reopen database.." << endl;

      reopen ();

      s = notmuch_database_find_message (nm_db, mid.c_str(), &msg);

      if (s != NOTMUCH_STATUS_SUCCESS) {
        /* could not do that, failing */
        cout << "db: to no avail, failure is iminent." << endl;

        throw database_error ("db: could not find message.");

      }
    }

    /* call function */
    func (msg);

    /* free resources */
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
    const char * ti = notmuch_thread_get_thread_id (t);
    if (ti == NULL) {
      throw database_error ("nmt: NULL thread_id");
    }

    thread_id = ti;

    load (t);
  }

  NotmuchThread::~NotmuchThread () {
  }

  void NotmuchThread::refresh (Db * db) {
    /* do a new db query and update all fields */

    lock_guard <Db> grd (*db);
    db->on_thread (thread_id,
        [&](notmuch_thread_t * nm_thread) {

          load (nm_thread);

        });
  }

  void NotmuchThread::load (notmuch_thread_t * nm_thread) {
    unread     = false;
    attachment = false;
    flagged    = false;

    /* update values */
    const char * s = notmuch_thread_get_subject (nm_thread);

    if (s != NULL)
      subject = ustring (s);

    newest_date = notmuch_thread_get_newest_date (nm_thread);
    total_messages = check_total_messages (nm_thread);
    authors     = get_authors (nm_thread);
    tags        = get_tags (nm_thread);
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

      if (tag != NULL) {
        if (string(tag) == "unread") {
          unread = true;
        } else if (string(tag) == "attachment") {
          attachment = true;
        } else if (string(tag) == "flagged") {
          flagged = true;
        }

        ttags.push_back (ustring(tag));
      }
    }

    return ttags;
  }

  /* get and split authors */
  vector<ustring> NotmuchThread::get_authors (notmuch_thread_t * nm_thread) {
    const char * auths = notmuch_thread_get_authors (nm_thread);

    ustring astr;

    if (auths != NULL) {
      astr = auths;
    } else {
      cerr << "nmt: got NULL for authors." << endl;
    }

    vector<ustring> aths = VectorUtils::split_and_trim (astr, ",");

    return aths;
  }

  int NotmuchThread::check_total_messages (notmuch_thread_t * nm_thread) {
    int c = notmuch_thread_get_total_messages (nm_thread);
    return c;
  }

  /* tag actions */
  bool NotmuchThread::add_tag (Db * db, ustring tag) {
    lock_guard <Db> grd (*db);

    cout << "nm (" << thread_id << "): add tag: " << tag << endl;
    tag = sanitize_tag (tag);
    if (!check_tag (tag)) {
      cout << "nm (" << thread_id << "): error, invalid tag: " << tag << endl;
      return false;
    }

    bool res = true;

    db->on_thread (thread_id, [&](notmuch_thread_t * nm_thread)
      {
        if (find(tags.begin (), tags.end (), tag) == tags.end ()) {
          /* get messages from thread */
          notmuch_messages_t * qmessages;
          notmuch_message_t  * message;


          for (qmessages = notmuch_thread_get_messages (nm_thread);
               notmuch_messages_valid (qmessages);
               notmuch_messages_move_to_next (qmessages)) {

            message = notmuch_messages_get (qmessages);

            notmuch_status_t s = notmuch_message_add_tag (message, tag.c_str ());

            if (s == NOTMUCH_STATUS_SUCCESS) {
              res &= true;
            } else {
              cerr << "nm: could not add tag: " << tag << " to thread: " << thread_id << endl;
              res = false;
              return;
            }

          }

          if (res) {
            tags.push_back (tag);

            // add to global tag list
            if (find(db->tags.begin (),
                  db->tags.end (),
                  tag) == db->tags.end ()) {
              db->tags.push_back (tag);
            }
          }

          res = true;
        } else {
          res = false;
        }
      });

    return res;
  }

  bool NotmuchThread::remove_tag (Db * db, ustring tag) {
    cout << "nm (" << thread_id << "): remove tag: " << tag << endl;
    tag = sanitize_tag (tag);
    if (!check_tag (tag)) {
      cout << "nm (" << thread_id << "): error, invalid tag: " << tag << endl;
      return false;
    }

    bool res = true;
    db->on_thread (thread_id, [&](notmuch_thread_t * nm_thread)
      {

        if (find(tags.begin (), tags.end (), tag) != tags.end ()) {

          /* get messages from thread */
          notmuch_messages_t * qmessages;
          notmuch_message_t  * message;


          for (qmessages = notmuch_thread_get_messages (nm_thread);
               notmuch_messages_valid (qmessages);
               notmuch_messages_move_to_next (qmessages)) {

            message = notmuch_messages_get (qmessages);

            notmuch_status_t s = notmuch_message_remove_tag (message, tag.c_str ());

            if (s == NOTMUCH_STATUS_SUCCESS) {
              res &= true;
            } else {
              cerr << "nm: could not remove tag: " << tag << " from thread: " << thread_id << endl;
              res = false;
              return;
            }

          }

          if (res) {
            tags.erase (remove (tags.begin (),
                                tags.end (),
                                tag), tags.end ());
          }

          res = true;
        } else {
          cout << "nm: thread does not have tag." << endl;
          res = false;
        }
      });

    return res;
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

