# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/liststore.h>

# include <notmuch.h>

# include "mode.hh"


using namespace std;

namespace Gulp {

  class ThreadIndex : public Mode  {
    public:
      ThreadIndex (string);
      ~ThreadIndex ();

      void add_threads ();

      virtual void grab_modal () override;

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView  * list_view;

      string query_string;
      notmuch_query_t * query;
  };
}
