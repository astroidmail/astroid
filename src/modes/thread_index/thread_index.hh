# pragma once

# include <vector>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/liststore.h>
# include <gtkmm/scrolledwindow.h>

# include <notmuch.h>

# include "modes/paned_mode.hh"

namespace Astroid {

  class ThreadIndex : public PanedMode  {
    public:
      ThreadIndex (MainWindow *, ustring, ustring = "");
      ~ThreadIndex ();

      unsigned int total_messages;
      unsigned int unread_messages;

      int thread_load_step;             /* configurable */
      int current_threads_loaded  = 0;

      void load_more_threads (bool all = false, int count = -1);
      void refresh (bool all, int count);
      void refresh_stats (Db *);
      int reopen_tries = 0;

      void open_thread (refptr<NotmuchThread>, bool new_tab, bool new_window = false);
      ThreadView * thread_view;
      bool thread_view_loaded  = false;
      bool thread_view_visible = false;

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView * list_view;
      ThreadIndexScrolled * scroll;

      ustring name = ""; // used as title for default queries
      ustring query_string;

      virtual ustring get_label () override;

    private:
      notmuch_sort_t sort;
      std::vector<ustring> sort_strings = { "oldest", "newest", "messageid", "unsorted" };
  };
}
