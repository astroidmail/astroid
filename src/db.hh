# pragma once

# include <notmuch.h>

# include "proto.hh"

using namespace std;

namespace Gulp {
  class NotmuchThread {
    public:
      NotmuchThread ();
      NotmuchThread (notmuch_thread_t *);
      notmuch_thread_t * nm_thread;
  };

  class Db {
    public:
      Db ();
      ~Db ();

      notmuch_database_t * nm_db;

      void test_query ();

  };
}

