# pragma once

# include <gtkmm.h>
# include <gtkmm/liststore.h>
# include <gtkmm/treeview.h>

# include "proto.hh"
# include "config.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  /* the list view consists of:
   * - a scolled window (which may be paned)
   * - a Treeview
   * - a ListStore
   */

  /* ----------
   * list store
   * ----------
   */
  class ThreadIndexListStore : public Gtk::ListStore {
    public:
      class ThreadIndexListStoreColumnRecord : public Gtk::TreeModel::ColumnRecord
      {
        public:
          Gtk::TreeModelColumn<time_t> newest_date;
          Gtk::TreeModelColumn<Glib::ustring> thread_id;
          Gtk::TreeModelColumn<Glib::RefPtr<NotmuchThread>> thread;

          ThreadIndexListStoreColumnRecord ();
      };

      ThreadIndexListStore ();
      ~ThreadIndexListStore ();
      const ThreadIndexListStoreColumnRecord columns;
  };


  /* ---------
   * list view
   * ---------
   */
  class ThreadIndexListView : public Gtk::TreeView {
    public:
      ThreadIndexListView (ThreadIndex *, Glib::RefPtr<ThreadIndexListStore>);
      ~ThreadIndexListView ();

      ptree config;
      bool  open_paned_default = true;

      ThreadIndex * thread_index;
      MainWindow  * main_window;
      Glib::RefPtr<ThreadIndexListStore> list_store;

      ThreadIndexListCellRenderer * renderer;
      int page_jump_rows; // rows to jump

      void set_thread_data (Gtk::CellRenderer *, const Gtk::TreeIter & );

      ustring get_current_thread_id ();
      refptr<NotmuchThread> get_current_thread ();

      virtual ModeHelpInfo * key_help ();

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
      void on_my_row_activated  (const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *);

      void on_thread_updated (Db *, ustring);
      void on_refreshed ();
  };


  /* ----------
   * scrolled window
   * ----------
   */
  class ThreadIndexScrolled : public Mode {
    public:
      ThreadIndexScrolled (MainWindow *,
                           Glib::RefPtr<ThreadIndexListStore> list_store,
                           ThreadIndexListView * list_view);
      ~ThreadIndexScrolled ();

      Glib::RefPtr<ThreadIndexListStore>  list_store;
      ThreadIndexListView * list_view;

      Gtk::ScrolledWindow scroll;

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

