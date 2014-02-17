# pragma once

# include <gtkmm.h>
# include <gtkmm/liststore.h>
# include <gtkmm/treeview.h>

# include "../proto.hh"

using namespace std;

namespace Gulp {
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
          Gtk::TreeModelColumn<Glib::ustring> thread_id;
          Gtk::TreeModelColumn<Glib::RefPtr<NotmuchThread>> thread;

          ThreadIndexListStoreColumnRecord ();
      };

      ThreadIndexListStore ();
      const ThreadIndexListStoreColumnRecord columns;
  };


  /* ---------
   * list view
   * ---------
   */
  class ThreadIndexListView : public Gtk::TreeView {
    public:
      ThreadIndexListView (Glib::RefPtr<ThreadIndexListStore>);

      Glib::RefPtr<ThreadIndexListStore> list_store;

      void set_thread_data (Gtk::CellRenderer *, const Gtk::TreeIter & );

      ThreadIndex * get_parent_thread_index ();

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };


  /* ----------
   * scrolled window
   * ----------
   */
  class ThreadIndexScrolled : public Gtk::ScrolledWindow, public PanedChild {
    public:
      ThreadIndexScrolled (Glib::RefPtr<ThreadIndexListStore> list_store,
                           ThreadIndexListView * list_view);

      Glib::RefPtr<ThreadIndexListStore>  list_store;
      ThreadIndexListView * list_view;

      /* paned child */
      virtual void grab_modal () override;
      virtual void release_modal () override;
      virtual Gtk::Widget * get_widget () override;
  };
}

