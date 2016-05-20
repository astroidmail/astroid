# include <iostream>
# include <vector>
# include <algorithm>
# include <exception>
# include <boost/filesystem.hpp>

# include <thread>
# include <chrono>
# include <atomic>
# include <condition_variable>
# include <mutex>

# include <glibmm.h>

# include <notmuch.h>

# include "db.hh"
# include "utils/utils.hh"
# include "utils/address.hh"
# include "log.hh"
# include "actions/action_manager.hh"
# include "message_thread.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  std::atomic<int>          Db::read_only_dbs_open;
  std::mutex                Db::db_open;
  std::condition_variable   Db::dbs_open;

  /* static settings */
  bool Db::maildir_synchronize_flags = false;
  std::vector<ustring> Db::excluded_tags = { "muted", "spam", "deleted" };
  std::vector<ustring> Db::sent_tags = { "sent" };
  std::vector<ustring> Db::draft_tags = { "draft" };
  std::vector<ustring> Db::tags;

  bfs::path Db::path_db;

  void Db::init () {
    const ptree& config = astroid->notmuch_config ();

    const char * home = getenv ("HOME");
    if (home == NULL) {
      log << error << "db: error: HOME variable not set." << endl;
      throw invalid_argument ("db: error: HOME environment variable not set.");
    }

    ustring db_path = ustring (config.get<string> ("database.path"));

    /* replace ~ with home */
    if (db_path[0] == '~') {
      path_db = path(home) / path(db_path.substr(1,db_path.size()));
    } else {
      path_db = path(db_path);
    }

    path_db = absolute (path_db);

    ustring excluded_tags_s = config.get<string> ("search.exclude_tags");
    excluded_tags = VectorUtils::split_and_trim (excluded_tags_s, ";");
    sort (excluded_tags.begin (), excluded_tags.end ());

    // TODO: find a better way to handle sent_tags, probably via AccountManager?
    ustring sent_tags_s = astroid->config().get<std::string> ("mail.sent_tags");
    sent_tags = VectorUtils::split_and_trim (sent_tags_s, ",");
    sort (sent_tags.begin (), sent_tags.end ());

    maildir_synchronize_flags = config.get<bool> ("maildir.synchronize_flags");
  }

  Db::Db (DbMode _mode) {
    mode = _mode;

    time_t start = clock ();

    nm_db     = NULL;
    if (mode == DATABASE_READ_ONLY) {
      open_db_read_only ();
    } else if (mode == DATABASE_READ_WRITE) {
      open_db_write (true);
    } else {
      throw invalid_argument ("db: mode must be read-only or read-write");
    }

    float diff = (clock () - start) * 1000.0 / CLOCKS_PER_SEC;
    log << debug << "db: open time: " << diff << " ms." << endl;
  }

  bool Db::open_db_write (bool block) {
    log << info << "db: open db read-write." << endl;

    /* lock will wait for all read-onlys to close, lk will not be released before
     * db is closed */
    rw_lock = Db::acquire_rw_lock ();

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
        log << error << "db: error: could not open db r/w, waited " <<
                time << " of maximum " <<
                db_write_open_timeout << " seconds." << endl;

        chrono::seconds duration (db_write_open_delay);
        this_thread::sleep_for (duration);

        time += db_write_open_delay;

      }

    } while (block && (s == NOTMUCH_STATUS_XAPIAN_EXCEPTION) && (time <= db_write_open_timeout));

    if (s != NOTMUCH_STATUS_SUCCESS) {
      log << error << "db: error: failed opening database for writing, have you configured the notmuch database path correctly?" << endl;

      release_rw_lock (rw_lock);
      throw database_error ("failed to open database for writing");


      return false;
    }

    return true;
  }

  bool Db::open_db_read_only () {
    Db::acquire_ro_lock ();

    notmuch_status_t s =
      notmuch_database_open (
        path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
        &nm_db);

    if (s != NOTMUCH_STATUS_SUCCESS) {
      log << error << "db: error: failed opening database for reading, have you configured the notmuch database path correctly?" << endl;

      throw database_error ("failed to open database (read-only)");


      return false;
    }

    return true;
  }

  std::unique_lock<std::mutex> Db::acquire_rw_lock () {
    /* lock will wait for all read-onlys to close, lk will not be released before
     * db is closed */
    log << debug << "db: rw-s: waiting for rw lock.. (r-o open: " << read_only_dbs_open << ")" << endl;
    std::unique_lock<std::mutex> rwl (db_open);
    dbs_open.wait (rwl, [] { return (read_only_dbs_open == 0); });
    log << debug << "db: rw-s lock acquired." << endl;

    return rwl;
  }

  void Db::release_rw_lock (std::unique_lock<std::mutex> &rwl) {
    log << debug << "db: rw-s: releasing lock." << endl;
    rwl.unlock ();
    dbs_open.notify_all ();
  }

  void Db::acquire_ro_lock () {
    log << info << "db: open db read-only, waiting for lock.." << endl;

    /* will block if there is an read-write db open */
    std::lock_guard<std::mutex> lk (db_open);
    read_only_dbs_open++;
    log << debug << "db: read-only got lock." << endl;
  }

  void Db::release_ro_lock () {
    log << debug << "db: ro: waiting for lock to close.." << endl;
    std::lock_guard<std::mutex> lk (db_open);
    log << debug << "db: ro: closing.." << endl;
    read_only_dbs_open--;
    dbs_open.notify_all ();
  }

  void Db::close () {
    if (!closed) {
      closed = true;

      if (nm_db != NULL) {
        log << info << "db: closing db." << endl;
        notmuch_database_close (nm_db);
        nm_db = NULL;
      }

      if (mode == DATABASE_READ_WRITE) {
        log << debug << "db: rw: releasing lock." << endl;
        release_rw_lock (rw_lock);
      } else {
        release_ro_lock ();
      }
    }
  }

  Db::~Db () {
    close ();
  }

# ifdef HAVE_NOTMUCH_GET_REV
  unsigned long Db::get_revision () {
    const char *uuid;
    unsigned long revision = notmuch_database_get_revision (nm_db, &uuid);

    return revision;
  }

# endif

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

    log << info << "db: loaded " << tags.size () << " tags." << endl;
  }

  bool Db::remove_message (ustring fname) {
    notmuch_status_t s = notmuch_database_remove_message (nm_db,
        fname.c_str ());

    if ((s != NOTMUCH_STATUS_SUCCESS) && (s != NOTMUCH_STATUS_DUPLICATE_MESSAGE_ID))
    {
      log << error << "db: error removing message: " << s << endl;
      throw database_error ("db: could not remove message from database.");
    }

    return (s != NOTMUCH_STATUS_DUPLICATE_MESSAGE_ID);
  }

  ustring Db::add_message_with_tags (ustring fname, vector<ustring> tags) {
    notmuch_message_t * msg;

    notmuch_status_t s = notmuch_database_add_message (nm_db,
        fname.c_str (),
        &msg);

    if ((s != NOTMUCH_STATUS_SUCCESS) && (s != NOTMUCH_STATUS_DUPLICATE_MESSAGE_ID)) {
      log << error << "db: error adding message: " << s << endl;

      if (s == NOTMUCH_STATUS_FILE_ERROR) {
        log << error << "db: file seems to have been moved, ignoring - probably a race condition with some syncing program." << endl;
        return "";
      } else {
        throw database_error ("db: could not add message to database.");
      }
    }

    /* add tags */
    for (ustring &t : tags) {
      s = notmuch_message_add_tag (msg, t.c_str());
    }

    if ((s == NOTMUCH_STATUS_SUCCESS) && maildir_synchronize_flags) {
      s = notmuch_message_tags_to_maildir_flags (msg);
    }

    /* emit signal */
    const char * mid = notmuch_message_get_message_id (msg);
    ustring _mid;

    if (mid != NULL) {
      _mid = ustring (mid);
    }

    notmuch_message_destroy (msg);

    return _mid;
  }

  ustring Db::add_sent_message (ustring fname, vector<ustring> additional_sent_tags) {
    log << info << "db: adding sent message: " << fname << endl;
    additional_sent_tags.insert (additional_sent_tags.end (), sent_tags.begin (), sent_tags.end ());
    additional_sent_tags.erase (unique (additional_sent_tags.begin (),
          additional_sent_tags.end ()),
          additional_sent_tags.end ());


    return add_message_with_tags (fname, additional_sent_tags);
  }

  ustring Db::add_draft_message (ustring fname) {
    log << info << "db: adding draft message: " << fname << endl;
    return add_message_with_tags (fname, draft_tags);
  }


  bool Db::thread_in_query (ustring query_in, ustring thread_id) {
    /* check if thread id is in query */
    string query_s;

    UstringUtils::trim(query_in);

    if (query_in.length() == 0 || query_in == "*") {
      query_s = "thread:" + thread_id;
    } else {
      query_s = "thread:" + thread_id  + " AND (" + query_in + ")";
    }

    time_t t0 = clock ();

    log << debug << "db: checking if thread: " << thread_id << " matches query: " << query_in << endl;

    notmuch_query_t * query = notmuch_query_create (nm_db, query_s.c_str());
    for (ustring &t : excluded_tags) {
      notmuch_query_add_tag_exclude (query, t.c_str());
    }
    notmuch_query_set_omit_excluded (query, NOTMUCH_EXCLUDE_TRUE);

    unsigned int c;
    notmuch_status_t st = notmuch_query_count_threads_st (query, &c);

    if (c > 1) {
      throw database_error ("db: got more than one thread for thread id.");
    }

    /* free resources */
    notmuch_query_destroy (query);

    log << debug << "db: thread in query check: " << ((clock() - t0) * 1000.0 / CLOCKS_PER_SEC) << " ms." << endl;

    if (c == 1) return (st == NOTMUCH_STATUS_SUCCESS);
    else        return false;

  }

  void Db::on_thread (ustring thread_id, function<void(notmuch_thread_t *)> func) {

    string query_s = "thread:" + thread_id;

    notmuch_query_t * query = notmuch_query_create (nm_db, query_s.c_str());
    notmuch_thread_t  * nm_thread;
    notmuch_threads_t * nm_threads;
    notmuch_status_t st;

    st = notmuch_query_search_threads_st (query, &nm_threads);

    if ((st != NOTMUCH_STATUS_SUCCESS) || nm_threads == NULL) {
      notmuch_query_destroy (query);
      log << error << "db: could not find thread: " << thread_id << ", status: " << st << endl;

      func (NULL);

      return;
    }

    int c = 0;
    for ( ; notmuch_threads_valid (nm_threads);
         notmuch_threads_move_to_next (nm_threads)) {
      if (c > 0) {
        log << error << "db: got more than one thread for thread id." << endl;
        throw invalid_argument ("db: got more than one thread for thread id.");
      }

      nm_thread = notmuch_threads_get (nm_threads);

      c++;
    }

    if (c < 1) {
      log << error << "db: could not find thread: " << thread_id << endl;
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

    if (msg == NULL || s != NOTMUCH_STATUS_SUCCESS) {
      notmuch_message_destroy (msg);
      log << error << "db: could not find message: " << mid << ", status: " << s << endl;

      return;
    }

    /* call function */
    func (msg);

    /* free resources */
    notmuch_message_destroy (msg);
  }

  void Db::test_query () { // {{{
    log << info << "db: running test query.." << endl;
    auto q = notmuch_query_create (nm_db, "label:inbox");

    unsigned int c;
    /* notmuch_status_t st = */ notmuch_query_count_messages_st (q, &c);

    log << info << "query: " << notmuch_query_get_query_string (q) << ", approx: "
         << c << " messages." << endl;

    notmuch_messages_t * messages;
    notmuch_message_t  * message;
    notmuch_status_t st;

    for (st = notmuch_query_search_messages_st (q, &messages);
         (st == NOTMUCH_STATUS_SUCCESS) && notmuch_messages_valid (messages);
         notmuch_messages_move_to_next (messages)) {
      message = notmuch_messages_get (messages);

      log << info << "thread:" << notmuch_message_get_thread_id (message) << ", message: " << notmuch_message_get_header (message, "Subject") << endl;

      notmuch_message_destroy (message);
    }
  } // }}}

  ustring Db::sanitize_tag (ustring tag) {
    UstringUtils::trim (tag);

    return tag;
  }

  bool Db::check_tag (ustring tag) {
    if (tag.empty()) {
      log << error << "nmt: invalid tag, empty." << endl;
      return false;
    }

    const vector<ustring> invalid_chars = { "\"" };

    for (const ustring &c : invalid_chars) {
      if (tag.find (c) != ustring::npos) {
        log << error << "nmt: invalid char in tag: " << c << endl;
        return false;
      }
    }

    if (tag.size() > NOTMUCH_TAG_MAX) {
      log << error << "nmt: error: maximum tag length is: " << NOTMUCH_TAG_MAX << endl;
      return false;
    }

    return true;
  }


  /* --------------
   * notmuch thread
   * --------------
   */
  NotmuchThread::NotmuchThread (notmuch_thread_t * t) {
    const char * ti = notmuch_thread_get_thread_id (t);
    if (ti == NULL) {
      log << error << "nmt: got NULL thread id." << endl;
      throw database_error ("nmt: NULL thread_id");
    }

    thread_id = ti;

    load (t);
  }

  NotmuchThread::~NotmuchThread () {
    //log << debug << "nmt: deconstruct." << endl;
  }

  void NotmuchThread::refresh (Db * db) {
    /* do a new db query and update all fields */

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
    const char * s = notmuch_thread_get_subject (nm_thread); // belongs to thread

    if (s != NULL) {
      subject = ustring (s);
    }

    newest_date = notmuch_thread_get_newest_date (nm_thread);
    oldest_date = notmuch_thread_get_oldest_date (nm_thread);
    total_messages = check_total_messages (nm_thread);
    tags        = get_tags (nm_thread);
    authors     = get_authors (nm_thread);
  }

  vector<ustring> NotmuchThread::get_tags (notmuch_thread_t * nm_thread) {

    notmuch_tags_t *  tags;
    const char *      tag;

    vector<ustring> ttags;

    for (tags = notmuch_thread_get_tags (nm_thread);
         notmuch_tags_valid (tags);
         notmuch_tags_move_to_next (tags))
    {
      tag = notmuch_tags_get (tags); // tag belongs to tags

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

    notmuch_tags_destroy (tags);

    sort (ttags.begin (), ttags.end ());

    return ttags;
  }

  vector<tuple<ustring,bool>> NotmuchThread::get_authors (notmuch_thread_t * nm_thread) {
    /* important: this might be called from another thread, we cannot output anything here */

    /* returns a vector of authors and whether they are authors of
     * an unread message in the thread */
    vector<tuple<ustring, bool>> aths;

    /* first check if any messages are unread, if not: just fetch the authors for
     * the thread without checking each one.
     *
     * `get_tags ()` have already been called, so we can safely use `unread` */

    if (!unread) {
      const char * auths = notmuch_thread_get_authors (nm_thread);

      ustring astr;

      if (auths != NULL) {
        astr = auths;
      } else {
        /* log << error << "nmt: got NULL for authors!" << endl; */
      }

      std::vector<ustring> maths = VectorUtils::split_and_trim (astr, ",|\\|");

      for (auto & a : maths) {
        aths.push_back (make_tuple (a, false));
      }

      return aths;
    }

    /* get messages from thread */
    notmuch_messages_t * qmessages;
    notmuch_message_t  * message;

    bool _unread;

    for (qmessages = notmuch_thread_get_messages (nm_thread);
         notmuch_messages_valid (qmessages);
         notmuch_messages_move_to_next (qmessages)) {

      message = notmuch_messages_get (qmessages);

      ustring a;
      const char * ac = notmuch_message_get_header (message, "From");
      if (ac != NULL) {
        a = Address(ustring (ac)).fail_safe_name ();
      } else {
        /* log << error << "nmt: got NULL for author!" << endl; */
        continue;
      }

      _unread = false;

      /* get tags */
      notmuch_tags_t *tags;
      const char *tag;

      for (tags = notmuch_message_get_tags (message);
           notmuch_tags_valid (tags);
           notmuch_tags_move_to_next (tags))
      {
          tag = notmuch_tags_get (tags);
          if (ustring (tag) == "unread")
          {
            _unread = true;
            break;
          }
      }

      auto fnd = find_if (aths.begin (), aths.end (),
          [&] (tuple<ustring, bool> &p) {
            return get<0>(p) == a;
          });

      if (fnd == aths.end ()) {
        aths.push_back (make_tuple (a, _unread));
      } else {
        /* check if it is marked unread */
        if (_unread && !(get<1> (*fnd))) {
          get<1> (*fnd) = true;
        }
      }

      notmuch_message_destroy (message);
    }

    return aths;
  }

  int NotmuchThread::check_total_messages (notmuch_thread_t * nm_thread) {
    int c = notmuch_thread_get_total_messages (nm_thread);
    return c;
  }

  /* tag actions */
  bool NotmuchThread::add_tag (Db * db, ustring tag) {
    log << debug << "nm (" << thread_id << "): add tag: " << tag << endl;
    tag = Db::sanitize_tag (tag);
    if (!Db::check_tag (tag)) {
      log << debug << "nm (" << thread_id << "): error, invalid tag: " << tag << endl;
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

            if ((s == NOTMUCH_STATUS_SUCCESS) && db->maildir_synchronize_flags) {
              s = notmuch_message_tags_to_maildir_flags (message);
            }

            notmuch_message_destroy (message);

            if (s == NOTMUCH_STATUS_SUCCESS) {
              res &= true;
            } else {
              log << error << "nm: could not add tag: " << tag << " to thread: " << thread_id << endl;
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
    log << debug << "nm (" << thread_id << "): remove tag: " << tag << endl;
    tag = Db::sanitize_tag (tag);
    if (!Db::check_tag (tag)) {
      log << debug << "nm (" << thread_id << "): error, invalid tag: " << tag << endl;
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

            if ((s == NOTMUCH_STATUS_SUCCESS) && db->maildir_synchronize_flags) {
              s = notmuch_message_tags_to_maildir_flags (message);
            }

            notmuch_message_destroy (message);

            if (s == NOTMUCH_STATUS_SUCCESS) {
              res &= true;
            } else {
              log << error << "nm: could not remove tag: " << tag << " from thread: " << thread_id << endl;
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
          log << warn << "nm: thread does not have tag." << endl;
          res = false;
        }
      });

    return res;
  }


  void NotmuchThread::emit_updated (Db * db) {
    astroid->actions->emit_thread_updated (db, thread_id);
  }

  ustring NotmuchThread::str () {
    return "threadid:" + thread_id;
  }

  /****************
   * NotmuchMessage
   ****************/
  NotmuchMessage::NotmuchMessage (refptr<Message> m) {
    mid  = m->mid;
    tags = m->tags;
  }

  /* tag actions */
  bool NotmuchMessage::add_tag (Db * db, ustring tag) {
    log << debug << "nm (" << mid << "): add tag: " << tag << endl;
    tag = Db::sanitize_tag (tag);
    if (!Db::check_tag (tag)) {
      log << debug << "nm (" << mid << "): error, invalid tag: " << tag << endl;
      return false;
    }

    bool res = true;

    db->on_message (mid, [&](notmuch_message_t * message)
      {
        notmuch_status_t s = notmuch_message_add_tag (message, tag.c_str ());

        if ((s == NOTMUCH_STATUS_SUCCESS) && db->maildir_synchronize_flags) {
          s = notmuch_message_tags_to_maildir_flags (message);
        }

        if (s == NOTMUCH_STATUS_SUCCESS) {
          res &= true;
        } else {
          log << error << "nm: could not add tag: " << tag << " to message: " << mid << endl;
          res = false;
          return;
        }

        /*
        if (res) {
          tags.push_back (tag);

          // add to global tag list
          if (find(db->tags.begin (),
                db->tags.end (),
                tag) == db->tags.end ()) {
            db->tags.push_back (tag);
          }
        }
        */
      });

    return res;
  }

  bool NotmuchMessage::remove_tag (Db * db, ustring tag) {
    log << debug << "nm (" << mid << "): remove tag: " << tag << endl;
    tag = Db::sanitize_tag (tag);
    if (!Db::check_tag (tag)) {
      log << debug << "nm (" << mid << "): error, invalid tag: " << tag << endl;
      return false;
    }

    bool res = true;

    db->on_message (mid, [&](notmuch_message_t * message)
      {
        notmuch_status_t s = notmuch_message_remove_tag (message, tag.c_str ());

        if ((s == NOTMUCH_STATUS_SUCCESS) && db->maildir_synchronize_flags) {
          s = notmuch_message_tags_to_maildir_flags (message);
        }

        if (s == NOTMUCH_STATUS_SUCCESS) {
          res &= true;
        } else {
          log << error << "nm: could not remove tag: " << tag << " from message: " << mid << endl;
          res = false;
          return;
        }

        /*
        if (res) {
          tags.push_back (tag);

          // add to global tag list
          if (find(db->tags.begin (),
                db->tags.end (),
                tag) == db->tags.end ()) {
            db->tags.push_back (tag);
          }
        }
        */
      });

    return res;
  }

  void NotmuchMessage::emit_updated (Db * db) {
    astroid->actions->emit_message_updated (db, mid);
  }

  ustring NotmuchMessage::str () {
    return "mid:" + mid;
  }


  /***************
   * NotmuchTaggable
   ***************/
  bool NotmuchTaggable::has_tag (ustring tag) {
    return (find(tags.begin (), tags.end (), tag) != tags.end ());
  }

  /***************
   * Exceptions
   ***************/

  database_error::database_error (const char * w) : runtime_error (w)
  {
  }


}

