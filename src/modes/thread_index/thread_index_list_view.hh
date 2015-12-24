# pragma once

# include <chrono>

# include <gtkmm.h>
# include <gtkmm/liststore.h>
# include <gtkmm/treeview.h>

# include "proto.hh"
# include "config.hh"
# include "modes/mode.hh"
# include "modes/keybindings.hh"

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
          Gtk::TreeModelColumn<bool> marked;

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
      void update_bg_image ();

    protected:
      void register_keys ();

      bool old (GdkEventKey *);

      ustring multi_key_help = "t: toggle, a: archive, *: flag, N: toggle unread, S: toggle spam, C-m: mute";
      bool multi_key_handler (GdkEventKey *);
      void on_my_row_activated  (const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *);

      virtual bool on_button_press_event (GdkEventButton *) override;
      Gtk::Menu item_popup;

      enum PopupItem {
        Reply,
        Forward,
        Archive,
        Open,
        OpenNewWindow,
      };

      void popup_activate_generic (enum PopupItem);

      void on_thread_updated (Db *, ustring);
      void on_refreshed ();

    private:
      chrono::time_point<chrono::steady_clock> last_redraw;
      bool redraw ();
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

