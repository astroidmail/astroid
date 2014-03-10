# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/liststore.h>
# include <gtkmm/scrolledwindow.h>

# include <notmuch.h>

# include "paned_mode.hh"


using namespace std;

namespace Astroid {

  class ThreadIndex : public PanedMode  {
    public:
      ThreadIndex (MainWindow *, ustring);
      ~ThreadIndex ();

      MainWindow * main_window;

      int initial_max_threads = 20;
      int current_thread = 0;

      void load_more_threads ();

      void open_thread (ustring, bool);
      ThreadView * thread_view;
      bool thread_view_loaded  = false;
      bool thread_view_visible = false;

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView  * list_view;
      ThreadIndexScrolled  * scroll;

      ustring query_string;
      notmuch_query_t * query;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}
