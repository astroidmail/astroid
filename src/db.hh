# pragma once

# include <vector>

# include <time.h>

# include <glibmm.h>
# include <notmuch.h>

# include <astroid.hh>
# include "proto.hh"

using namespace std;

namespace Astroid {
  class NotmuchThread : public Glib::Object {
    public:
      NotmuchThread (notmuch_thread_t *);

      string thread_id;

      char * subject_chr;
      ustring subject;
      time_t  newest_date;
      bool    unread;
      bool    attachment;
      int     total_messages;
      vector<ustring> authors;

      void    ensure_valid ();

    private:
      bool    check_unread (notmuch_thread_t *);
      bool    check_attachment (notmuch_thread_t *);
      int     check_total_messages (notmuch_thread_t *);
      vector<ustring> get_authors (notmuch_thread_t *);
  };

  class Db {
    public:
      Db ();
      ~Db ();

      notmuch_database_t * nm_db;

      void test_query ();
  };
}

