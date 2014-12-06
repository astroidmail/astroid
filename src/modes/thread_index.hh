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

      int thread_load_step    = 150;
      int current_thread      = 0;

      void load_more_threads (bool all = false, int count = -1, bool checked = false);
      void refresh (bool, int, bool);
      void setup_query ();
      void close_query ();
      int reopen_tries = 0;

      void open_thread (refptr<NotmuchThread>, bool);
      ThreadView * thread_view;
      bool thread_view_loaded  = false;
      bool thread_view_visible = false;

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView * list_view;
      ThreadIndexScrolled * scroll;

      ustring query_string;

      Db * db = NULL;
      notmuch_query_t   * query;
      notmuch_threads_t * threads;

      virtual ModeHelpInfo * key_help ();

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;

  };
}
