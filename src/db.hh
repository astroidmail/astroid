# pragma once

# include <notmuch.h>

# include "proto.hh"

using namespace std;

namespace Gulp {
  class Db {
    public:
      Db ();
      ~Db ();

      notmuch_database_t * nm_db;

      void test_query ();

  };
}

