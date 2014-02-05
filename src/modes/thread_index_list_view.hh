# pragma once

# include <gtkmm.h>
# include <gtkmm/liststore.h>
# include <gtkmm/treeview.h>

# include "../proto.hh"

using namespace std;

namespace Gulp {
  class ThreadIndexListStore : public Gtk::ListStore {
    public:
      class ThreadIndexListStoreColumnRecord : public Gtk::TreeModel::ColumnRecord
      {
        public:
          Gtk::TreeModelColumn<Glib::ustring> thread_id;
          Gtk::TreeModelColumn<NotmuchThread> thread;

          ThreadIndexListStoreColumnRecord ();
      };

      ThreadIndexListStore ();
      const ThreadIndexListStoreColumnRecord columns;

  };

  class ThreadIndexListView : public Gtk::TreeView {
    public:
      ThreadIndexListView (Glib::RefPtr<ThreadIndexListStore>);

      Glib::RefPtr<ThreadIndexListStore> list_store;

      void set_thread_data (Gtk::CellRenderer *, const Gtk::TreeIter & );
  };
}

