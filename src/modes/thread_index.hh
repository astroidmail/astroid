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
      ThreadIndex (MainWindow *, string);
      ~ThreadIndex ();

      MainWindow * main_window;

      int initial_max_threads = 20;

      void add_threads ();

      void open_thread (ustring, bool);
      ThreadView * thread_view;
      bool thread_view_loaded  = false;
      bool thread_view_visible = false;

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView  * list_view;
      ThreadIndexScrolled  * scroll;

      string query_string;
      notmuch_query_t * query;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}
