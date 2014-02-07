# pragma once

# include <glibmm.h>
# include <notmuch.h>

# include <gulp.hh>
# include "proto.hh"

using namespace std;

namespace Gulp {
  class NotmuchThread {
    public:
      NotmuchThread ();
      NotmuchThread (notmuch_thread_t *);

      notmuch_thread_t * nm_thread;
      string thread_id;

      ustring * get_subject ();
  };

  class Db {
    public:
      Db ();
      ~Db ();

      notmuch_database_t * nm_db;

      void test_query ();

  };
}

