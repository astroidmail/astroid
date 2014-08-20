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
      ~NotmuchThread ();

      ustring thread_id;

      char * subject_chr;
      ustring subject;
      time_t  newest_date;
      bool    unread;
      bool    attachment;
      bool    flagged;
      int     total_messages;
      vector<ustring> authors;
      vector<ustring> tags;

      void refresh (Db *);
      void load (notmuch_thread_t *);

      bool remove_tag (Db *, ustring);
      bool add_tag (Db *, ustring);
      ustring sanitize_tag (ustring);
      bool check_tag (ustring);

    private:
      int     check_total_messages (notmuch_thread_t *);
      vector<ustring> get_authors (notmuch_thread_t *);
      vector<ustring> get_tags (notmuch_thread_t *);
  };

  class Db : public recursive_mutex {
    public:
      enum DbMode {
        DATABASE_READ_ONLY,
        DATABASE_READ_WRITE,
      };

      Db (DbMode);
      ~Db ();

      void reopen ();
      bool check_reopen (bool);

      void on_thread  (ustring, function <void(notmuch_thread_t *)>);
      void on_message (ustring, function <void(notmuch_message_t *)>);

    private:
      enum DbState {
        READ_ONLY,
        READ_WRITE,
        IN_CHANGE,
        CLOSED,
      };

      /* internal lock for open and close operations */
      atomic<DbState> db_state;

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

  /* exceptions */
  class database_error : public runtime_error {
    public:
      database_error (const char *);

  };
}

