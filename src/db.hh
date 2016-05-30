# pragma once

# include <mutex>
# include <condition_variable>
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
  class NotmuchTaggable : public Glib::Object {
    public:
      std::vector <ustring> tags;
      bool has_tag (ustring);

      virtual bool remove_tag (Db *, ustring) = 0;
      virtual bool add_tag (Db *, ustring) = 0;

      virtual void emit_updated (Db *) = 0;

      virtual ustring str () = 0;
  };

  /* the notmuch message object should get by on the db only */
  class NotmuchMessage : public NotmuchTaggable {
    public:
      NotmuchMessage (refptr<Message>);

      ustring mid;

      bool remove_tag (Db *, ustring) override;
      bool add_tag (Db *, ustring) override;
      void emit_updated (Db *) override;
      ustring str () override;
  };

  /* the notmuch thread object should get by on the db only */
  class NotmuchThread : public NotmuchTaggable {
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
      std::vector<std::tuple<ustring,bool>> authors;

      void refresh (Db *);
      void load (notmuch_thread_t *);

      bool remove_tag (Db *, ustring) override;
      bool add_tag (Db *, ustring) override;
      void emit_updated (Db *) override;
      ustring str () override;

    private:
      int check_total_messages (notmuch_thread_t *);
      std::vector<std::tuple<ustring,bool>> get_authors (notmuch_thread_t *);
      std::vector<ustring> get_tags (notmuch_thread_t *);
  };

  class Db {
    public:
      enum DbMode {
        DATABASE_READ_ONLY,
        DATABASE_READ_WRITE,
      };

      Db (DbMode = DATABASE_READ_ONLY);
      ~Db ();
      void close ();

      void on_thread  (ustring, std::function <void(notmuch_thread_t *)>);
      void on_message (ustring, std::function <void(notmuch_message_t *)>);

      bool thread_in_query (ustring, ustring);

# ifdef HAVE_NOTMUCH_GET_REV
      unsigned long get_revision ();
# endif

      notmuch_database_t * nm_db;

      static std::vector<ustring> tags;

      void load_tags ();

      static std::vector<ustring> sent_tags;
      static std::vector<ustring> draft_tags;
      static std::vector<ustring> excluded_tags;

      ustring add_sent_message (ustring, std::vector<ustring>);
      ustring add_draft_message (ustring);
      ustring add_message_with_tags (ustring fname, std::vector<ustring> tags);
      bool remove_message (ustring);

      static ustring sanitize_tag (ustring);
      static bool check_tag (ustring);

      /* lock db: use if you need the db in external program and need
       * a specific lock */
      static std::unique_lock<std::mutex> acquire_rw_lock ();
      static void release_rw_lock (std::unique_lock<std::mutex> &);

      static void acquire_ro_lock ();
      static void release_ro_lock ();

      static bool maildir_synchronize_flags;
      static void init ();
      static bfs::path path_db;

    private:
      /* we can have as many read-only db's open as we want, but only one
       * read-write at the same time. there can also not be any other
       * read-only db's open when there is a read-write open.
       *
       * if you open one read-only db, and try to open a read-write in the
       * same thread without closing the read-only there will be a deadlock.
       *
       */

      /* number of open read-only dbs, when 0 a read-write can be opened */
      static std::atomic<int>         read_only_dbs_open;

      /* synchronizes openings, read-write dbs will lock this for
       * its entire lifespan. read-only, only locks it while incrementing
       * the read_only_dbs_open */
      static std::mutex               db_open;

      /* notify when read_only_dbs change */
      static std::condition_variable  dbs_open;
      std::unique_lock<std::mutex>    rw_lock;

      DbMode mode;

      bool open_db_write (bool);
      bool open_db_read_only ();
      bool closed = false;

      const int db_write_open_timeout = 120; // seconds
      const int db_write_open_delay   = 1;   // seconds

  };

  /* exceptions */
  class database_error : public std::runtime_error {
    public:
      database_error (const char *);

  };
}

