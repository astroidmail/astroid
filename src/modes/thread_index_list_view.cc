# include <iostream>

# include "../db.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"

using namespace std;

namespace Gulp {
  /* list store */
  ThreadIndexListStore::ThreadIndexListStoreColumnRecord::ThreadIndexListStoreColumnRecord () {
    add (thread_id);
    add (thread);
  }

  ThreadIndexListStore::ThreadIndexListStore () {
    set_column_types (columns);
  }


  /* list view */
  ThreadIndexListView::ThreadIndexListView (Glib::RefPtr<ThreadIndexListStore> store) {
    list_store = store;

    set_model (list_store);
    set_enable_search (false);

    set_show_expanders (false);
    set_headers_visible (false);

    //append_column ("Thread IDs", list_store->columns.thread_id);

    /* add thread column */
    ThreadIndexListCellRenderer * renderer =
      Gtk::manage ( new ThreadIndexListCellRenderer () );
    int cols_count = append_column ("Thread", *renderer);
    Gtk::TreeViewColumn * column = get_column (cols_count - 1);

    column->set_cell_data_func (*renderer,
        sigc::mem_fun(*this, &ThreadIndexListView::set_thread_data) );
  }

  void ThreadIndexListView::set_thread_data (
      Gtk::CellRenderer * renderer,
      const Gtk::TreeIter &iter) {

    ThreadIndexListCellRenderer * r =
      (ThreadIndexListCellRenderer*) renderer;

    //cout << "setting thread.." << r <<  endl;
    if (iter) {

      Gtk::ListStore::Row row = *iter;
      r->thread = row[list_store->columns.thread];

    }
  }

  bool ThreadIndexListView::on_key_press_event (GdkEventKey *event) {
    switch (event->keyval) {
      case GDK_KEY_j:
      case GDK_KEY_J:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          path.next ();
          set_cursor (path);
          return true;
        }
        break;
      case GDK_KEY_k:
      case GDK_KEY_K:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          path.prev ();
          set_cursor (path);
          return true;
        }
        break;
    }

    return false;
  }
}

