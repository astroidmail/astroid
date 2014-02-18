# include <iostream>

# include "../db.hh"
# include "paned_mode.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"

using namespace std;

namespace Gulp {
  /* ----------
   * scrolled window
   * ----------
   */

  ThreadIndexScrolled::ThreadIndexScrolled (
      Glib::RefPtr<ThreadIndexListStore> _list_store,
      ThreadIndexListView * _list_view) {

    list_store = _list_store;
    list_view  = _list_view;

    tab_widget = new Gtk::Label ("None");

    scroll.add (*list_view);
    pack_start (scroll, true, true, 0);
    scroll.show_all ();
  }

  void ThreadIndexScrolled::grab_modal () {
    list_view->add_modal_grab ();
    list_view->grab_focus ();
  }

  void ThreadIndexScrolled::release_modal () {
    list_view->remove_modal_grab ();
  }

  /* ----------
   * list store
   * ----------
   */
  ThreadIndexListStore::ThreadIndexListStoreColumnRecord::ThreadIndexListStoreColumnRecord () {
    add (thread_id);
    add (thread);
  }

  ThreadIndexListStore::ThreadIndexListStore () {
    set_column_types (columns);
  }


  /* ---------
   * list view
   * ---------
   */
  ThreadIndexListView::ThreadIndexListView (ThreadIndex * _thread_index, Glib::RefPtr<ThreadIndexListStore> store) {
    thread_index = _thread_index;
    list_store = store;

    set_model (list_store);
    set_enable_search (false);

    set_show_expanders (false);
    set_headers_visible (false);
    set_grid_lines (Gtk::TreeViewGridLines::TREE_VIEW_GRID_LINES_NONE);

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
      case GDK_KEY_Down:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);

          if (path) {
            path.next ();
            set_cursor (path);
          } else {
            path.prev ();
          }

          return true;
        }
        break;

      case GDK_KEY_k:
      case GDK_KEY_K:
      case GDK_KEY_Up:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          path.prev ();
          if (path) {
            set_cursor (path);
          }
          return true;
        }
        break;

      case GDK_KEY_Return:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          get_cursor (path, c);
          Gtk::TreeIter iter;

          iter = list_store->get_iter (path);

          Gtk::ListStore::Row row = *iter;
          ustring thread_id = row[list_store->columns.thread_id];
          cout << "ti_list: loading: " << thread_id << endl;

          if (event->state & GDK_SHIFT_MASK) {
            /* open message in new tab */
            thread_index->open_thread (thread_id, true);


          } else {
            /* open message in split pane*/
            thread_index->open_thread (thread_id, false);

          }
        }
        return true;


    }

    return false;
  }
}

