# pragma once

# include <mutex>
# include <atomic>
# include <functional>

# include <vector>

# include <time.h>

# include <glibmm.h>
# include <notmuch.h>

# include "astroid.hh"
# include "config.hh"
# include "proto.hh"

namespace Astroid {
  /* the notmuch thread object should get by on the db only */
  class NotmuchThread : public Glib::Object {
    public:
      NotmuchThread (notmuch_thread_t *);
      ~NotmuchThread ();

      ustring thread_id;

      ustring subject;
      time_t  newest_date;
      time_t  oldest_date;
      bool    unread;
      bool    attachment;
      bool    flagged;
      int     total_messages;
      std::vector<ustring> authors;
      std::vector<ustring> tags;

      bool has_tag (ustring);

      void refresh (Db *);
      void load (notmuch_thread_t *);

      bool remove_tag (Db *, ustring);
      bool add_tag (Db *, ustring);
      ustring sanitize_tag (ustring);
      bool check_tag (ustring);

    private:
      int     check_total_messages (notmuch_thread_t *);
      std::vector<ustring> get_authors (notmuch_thread_t *);
      std::vector<ustring> get_tags (notmuch_thread_t *);
  };

  class Db : public std::recursive_mutex {
    public:
      enum DbMode {
        DATABASE_READ_ONLY,
        DATABASE_READ_WRITE,
      };

      Db (DbMode = DATABASE_READ_ONLY);
      ~Db ();

      void reopen ();
      bool check_reopen (bool);

      void on_thread  (ustring, std::function <void(notmuch_thread_t *)>);
      void on_message (ustring, std::function <void(notmuch_message_t *)>);

      bool thread_in_query (ustring, ustring);

# ifdef HAVE_NOTMUCH_GET_REV
      unsigned long get_revision ();
# endif

    private:
      enum DbState {
        READ_ONLY,
        READ_WRITE,
        IN_CHANGE,
        CLOSED,
      };

      /* internal lock for open and close operations */
      std::atomic<DbState> db_state;

      bool open_db_write (bool);
      bool open_db_read_only ();
      void close_db ();

      bfs::path path_db;
      const int db_write_open_timeout = 30; // seconds
      const int db_write_open_delay   = 1; // seconds

    public:
      notmuch_database_t * nm_db;

      ptree config;

      std::vector<ustring> tags;

      void load_tags ();
      void test_query ();

      std::vector<ustring> sent_tags = { "sent" };
      std::vector<ustring> draft_tags = { "draft" };
      std::vector<ustring> excluded_tags = { "muted", "spam", "deleted" };

      void add_sent_message (ustring, std::vector<ustring>);
      void add_draft_message (ustring);
      void add_message_with_tags (ustring fname, std::vector<ustring> tags);
      void remove_message (ustring);
  };

  /* exceptions */
  class database_error : public std::runtime_error {
    public:
      database_error (const char *);

  };
}

