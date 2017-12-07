# pragma once

# include <vector>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/liststore.h>
# include <gtkmm/scrolledwindow.h>

# include "modes/paned_mode.hh"
# include "query_loader.hh"
# include "modes/thread_view/thread_view.hh"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

namespace Astroid {

  class ThreadIndex : public PanedMode  {
    public:
      ThreadIndex (MainWindow *, ustring, ustring = "");
      ~ThreadIndex ();

      QueryLoader queryloader;

      void open_thread (refptr<NotmuchThread>, bool new_tab, bool new_window = false);

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView * list_view;
      ThreadIndexScrolled * scroll;

      ustring name = ""; // used as title for default queries
      ustring query_string;

      virtual ustring get_label () override;
      void pre_close () override;

# ifndef DISABLE_PLUGINS
      PluginManager::ThreadIndexExtension * plugins;
# endif

      void on_stats_ready ();

      bool on_index_action (ThreadView * tv, ThreadView::IndexAction);

    private:
      void on_first_thread_ready ();
  };
}
