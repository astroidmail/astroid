# pragma once

# include <vector>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/liststore.h>
# include <gtkmm/scrolledwindow.h>

# include "modes/paned_mode.hh"
# include "query_loader.hh"
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
      ThreadView * thread_view;
      bool thread_view_loaded  = false;
      bool thread_view_visible = false;

      Glib::RefPtr<ThreadIndexListStore> list_store;
      ThreadIndexListView * list_view;
      ThreadIndexScrolled * scroll;

      ustring name = ""; // used as title for default queries
      ustring query_string;

      virtual ustring get_label () override;
      void close (bool = false) override;

# ifndef DISABLE_PLUGINS
      PluginManager::ThreadIndexExtension * plugins;
# endif

    private:
      void on_stats_ready ();
      void on_first_thread_ready ();
  };
}
