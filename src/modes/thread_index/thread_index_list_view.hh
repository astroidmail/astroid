# pragma once

# include <chrono>

# include <gtkmm.h>
# include <gtkmm/liststore.h>
# include <gtkmm/treeview.h>

# include "proto.hh"
# include "config.hh"
# include "modes/mode.hh"
# include "modes/keybindings.hh"

# include "notmuch.h"

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
          Gtk::TreeModelColumn<time_t> oldest_date;
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

      ThreadIndex * thread_index;
      MainWindow  * main_window;
      refptr<ThreadIndexListStore> list_store;
      refptr<Gtk::TreeModelFilter> filtered_store;

      ThreadIndexListCellRenderer * renderer;
      int page_jump_rows; // rows to jump

      void set_thread_data (Gtk::CellRenderer *, const Gtk::TreeIter & );

      ustring get_current_thread_id ();
      refptr<NotmuchThread> get_current_thread ();

      void update_bg_image ();
      void set_sort_type (notmuch_sort_t sort);

      bool filter_visible_row ( const Gtk::TreeIter & iter );
      ustring              filter_txt;
      std::vector<ustring> filter;
      void on_filter (ustring k);


    protected:
      Keybindings multi_keys;
      void register_keys ();

      typedef enum {
        MUnread = 0,
        MFlag,
        MArchive,
        MSpam,
        MMute,
        MToggle,
        MTag,
      } multi_key_action;

      bool multi_key_handler (multi_key_action, Key);

      void on_my_row_activated  (const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *);

      virtual bool on_button_press_event (GdkEventButton *) override;
      Gtk::Menu item_popup;

      enum PopupItem {
        Reply,
        Forward,
        Flag,
        Archive,
        Open,
        OpenNewWindow,
      };

      void popup_activate_generic (enum PopupItem);

      // bypass scrolled window
      virtual bool on_key_press_event (GdkEventKey *) override;

    private:
      std::chrono::time_point<std::chrono::steady_clock> last_redraw;
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

