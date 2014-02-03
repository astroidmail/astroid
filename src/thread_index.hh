# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>

# include <notmuch.h>

# include "mode.hh"

using namespace std;

namespace Gulp {
  class ThreadIndex : public Mode  {
    public:
      ThreadIndex (string);
      ~ThreadIndex ();

      string query_string;
      notmuch_query_t * query;
  };
}
