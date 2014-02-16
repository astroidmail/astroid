# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/liststore.h>
# include <gtkmm/scrolledwindow.h>

# include <notmuch.h>

# include "paned_mode.hh"


using namespace std;

namespace Gulp {

  class ThreadIndex : public PanedMode  {
    public:
      ThreadIndex (string);
      ~ThreadIndex ();

      int initial_max_threads = 20;

      void add_threads ();

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView  * list_view;
      ThreadIndexScrolled  * scroll;

      string query_string;
      notmuch_query_t * query;
  };
}
