# pragma once

# include <time.h>

# include <glibmm.h>
# include <notmuch.h>

# include <gulp.hh>
# include "proto.hh"

using namespace std;

namespace Gulp {
  class NotmuchThread : public Glib::Object {
    public:
      NotmuchThread (notmuch_thread_t *);

      string              thread_id;

      char * subject_chr;
      ustring subject;
      time_t  newest_date;
      bool    unread;
      bool    attachment;

      void      ensure_valid ();

    private:
      bool      check_unread (notmuch_thread_t *);
      bool      check_attachment (notmuch_thread_t *);
  };

  class Db {
    public:
      Db ();
      ~Db ();

      notmuch_database_t * nm_db;

      void test_query ();

  };
}

