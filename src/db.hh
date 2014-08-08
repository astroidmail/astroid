# pragma once

# include <thread>
# include <mutex>
# include <atomic>
# include <condition_variable>

# include <vector>

# include <time.h>

# include <glibmm.h>
# include <notmuch.h>

# include "astroid.hh"
# include "config.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  /* the notmuch thread object should get by on the db only */
  class NotmuchThread : public Glib::Object {
    public:
      NotmuchThread (notmuch_thread_t *);
      NotmuchThread (ustring);
      ~NotmuchThread ();

      string thread_id;

      char * subject_chr;
      ustring subject;
      time_t  newest_date;
      bool    unread;
      bool    attachment;
      bool    flagged;
      int     total_messages;
      vector<ustring> authors;
      vector<ustring> tags;

      void refresh ();

      bool remove_tag (ustring);
      bool add_tag (ustring);
      ustring sanitize_tag (ustring);
      bool check_tag (ustring);

    private:
      int     check_total_messages ();
      vector<ustring> get_authors ();
      vector<ustring> get_tags ();

      /* activate valid db objects */
      Db * db;
      void activate ();
      void deactivate ();

      int ref = 0;
      notmuch_query_t *   query;
      notmuch_threads_t * nm_threads;
      notmuch_thread_t *  nm_thread;
  };

  class Db {
    public:
      Db ();
      ~Db ();

      /* the database should only be open in a read-write state
       * when necessary. all write-operations should be wrapped
       * in a write_lock which ensures that the database is open
       * in a read-write state. a lock or condition variable is
       * required for all db-operations to ensure that a database
       * re-open (change from read-only to read-write) is not in
       * progress.
       *
       * read and write operations may happen in parallel.
       *
       */

      void read_lock ();
      void read_release ();

      void write_lock ();
      void write_release ();

    private:
      enum DbState {
        READ_ONLY,
        READ_WRITE,
        IN_CHANGE,
        CLOSED,
      };

      mutex               db_ready_mut;
      condition_variable  db_ready_cv;

      /* internal lock for open and close operations */
      atomic<DbState> db_state;


      /* each time a write lock is requested this reference
       * count is incremented. if it is 0 upon reference
       * increment, a read-write connection must first be made.
       *
       * readers may use db_open_cv on db_state to check whether
       * the db is ready (READ_ONLY or READ_WRITE).
       *
       *
       */
      int                writers;
      mutex              writers_mux;
      condition_variable writers_cv;

      /* used by open_db methods to wait for all readers
       * to finish */
      int                 readers;
      mutex               readers_mux;
      condition_variable  readers_cv;


      bool open_db_write (bool);
      bool open_db_read_only ();
      void close_db ();

      path path_db;
      const int db_write_open_timeout = 30; // seconds
      const int db_write_open_delay   = 1; // seconds

    public:
      notmuch_database_t * nm_db;

      ptree config;

      vector<ustring> tags;

      void load_tags ();
      void test_query ();

      vector<ustring> sent_tags = { "sent" };
      void add_sent_message (ustring);
  };
}

