# include <iostream>
# include <string>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "astroid.hh"
# include "db.hh"
# include "modes/paned_mode.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "modes/thread_view/thread_view.hh"
# include "modes/saved_searches.hh"
# include "main_window.hh"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

using namespace std;

namespace Astroid {
  ThreadIndex::ThreadIndex (MainWindow *mw, ustring _query, ustring _name) : PanedMode(mw), query_string(_query) {

    name = _name;
    set_orientation (Gtk::Orientation::ORIENTATION_VERTICAL);

    /* set up treeview */
    list_store = Glib::RefPtr<ThreadIndexListStore>(new ThreadIndexListStore ());
    queryloader.list_store = list_store;

    list_view  = Gtk::manage(new ThreadIndexListView (this, list_store));
    queryloader.list_view = list_view;

    scroll     = Gtk::manage(new ThreadIndexScrolled (main_window, list_store, list_view));

    list_view->set_sort_type (queryloader.sort);

    add_pane (0, *scroll);

    show_all ();

    /* load threads */
    queryloader.stats_ready.connect (
        sigc::mem_fun (this, &ThreadIndex::on_stats_ready));
    queryloader.first_thread_ready.connect (
        sigc::mem_fun (this, &ThreadIndex::on_first_thread_ready));

    queryloader.start (query_string);

# ifndef DISABLE_PLUGINS
    plugins = new PluginManager::ThreadIndexExtension (this);
# endif

    set_label (get_label ());

    /* register keys {{{ */
    keys.set_prefix ("Thread Index", "thread_index");

    keys.register_key ("x", "thread_index.close_pane", "Close thread view pane if open",
        [&](Key) {
          if (current == 1) {
            if (thread_view_loaded && thread_view_visible) {
              /* hide thread view */
              del_pane (1);
              thread_view_visible = false;

              return true;
            }
          }

          return false;
        });


    keys.register_key (Key((guint) GDK_KEY_dollar), "thread_index.refresh", "Refresh query",
        [&] (Key) {
          queryloader.reload ();
          return true;
        });

    keys.register_key (Key (false, true, (guint) GDK_KEY_Tab), "thread_index.pane_swap_focus",
        "Swap focus to other pane if open",
        [&] (Key) {
          if (packed == 2) {
            release_modal ();
            current = (current == 0 ? 1 : 0);
            grab_modal ();
          }
          return true;
        });

    keys.register_key ("O", "thread_index.refine_query", "Refine query",
        [&] (Key) {
          if (!invincible) {
            main_window->enable_command (CommandBar::CommandMode::Search,
                query_string,
                [&](ustring new_query) {

                  query_string = new_query;
                  set_label (get_label ());
                  queryloader.refine_query (query_string);

                  /* add to saved searches */
                  SavedSearches::add_query_to_history (query_string);

                  /* select first */
                  list_view->set_cursor (Gtk::TreePath("0"));

                });

          } else {

            main_window->enable_command (CommandBar::CommandMode::Search, query_string, NULL);
          }

          return true;
        });

    keys.register_key ("C-v", "thread_index.duplicate_refine_query", "Duplicate and refine query",
        [&] (Key) {
          main_window->enable_command (CommandBar::CommandMode::Search, query_string, NULL);

          return true;
        });

    keys.register_key ("C-s", "thread_index.cycle_sort",
        "Cycle through sort options: 'oldest', 'newest', 'messageid', 'unsorted'",
        [&] (Key) {
          if (queryloader.sort == NOTMUCH_SORT_UNSORTED) {
            queryloader.sort = NOTMUCH_SORT_OLDEST_FIRST;
          } else {
            int s = static_cast<int> (queryloader.sort);
            s++;
            queryloader.sort = static_cast<notmuch_sort_t> (s);
          }

          LOG (info) << "ti: sorting by: " << queryloader.sort_strings[static_cast<int>(queryloader.sort)];

          list_view->set_sort_type (queryloader.sort);
          queryloader.reload ();
          return true;
        });

    keys.register_key ("C-u", { Key (true, false, (guint) GDK_KEY_Up), Key (GDK_KEY_Page_Up) },
        "thread_index.page_up",
        "Page up",
        [&] (Key) {
          auto adj = scroll->scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());

          /* check if cursor is in view */
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          list_view->get_cursor (path, c);

          /* not in view, move cursor to top */
          Gtk::TreePath newpath;
          int cx, cy;
          list_view->get_path_at_pos (0, 0, newpath, c, cx, cy);
          if (newpath == path) {
            newpath  = Gtk::TreePath("0");
          }
          if (newpath)
            list_view->set_cursor (newpath);

          return true;
        });

    keys.register_key ("C-d", { Key (true, false, (guint) GDK_KEY_Down), Key (GDK_KEY_Page_Down) },
        "thread_index.page_down",
        "Page down",
        [&] (Key) {
          auto adj = scroll->scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());

          /* check if cursor is in view */
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          list_view->get_cursor (path, c);

          /* not in view, move cursor to bottom */
          Gtk::TreePath newpath;
          int cx, cy;
          list_view->get_path_at_pos (0, list_view->get_height (), newpath, c, cx, cy);
          if (!newpath || newpath == path) {
            auto it = list_store->children().end ();
            newpath  = list_store->get_path (--it);
          }
          if (newpath)
            list_view->set_cursor (newpath);

          return true;
        });

    keys.register_key ("C-S",
        "thread_index.save_query",
        "Save query",
        [&] (Key) {
          SavedSearches::save_query (query_string);

          return true;
        });
    // }}}
  }

  void ThreadIndex::on_stats_ready () {
    LOG (debug) << "ti: got refresh stats.";
    set_label (get_label ());
    list_view->update_bg_image ();
  }

  void ThreadIndex::on_first_thread_ready () {
    /* select first */
    list_view->set_cursor (Gtk::TreePath("0"));
  }

  ustring ThreadIndex::get_label () {
    ustring f = "";
    if (!list_view->filter_txt.empty ()) {
      f = ustring::compose (" (%1: %2)", list_view->filter_txt, list_view->filtered_store->children ().size ());
    }

    if (name == "")
      return ustring::compose ("%1 (%2/%3)%4%5", query_string, queryloader.unread_messages,
          queryloader.total_messages, queryloader.loading() ? " (%)" : "",
          f
          );
    else
      return ustring::compose ("%1 (%2/%3)%4%5", name,
          queryloader.unread_messages, queryloader.total_messages, queryloader.loading() ? " (%)" : "",
          f
          );
  }

  void ThreadIndex::open_thread (refptr<NotmuchThread> thread, bool new_tab, bool new_window) {
    LOG (debug) << "ti: open thread: " << thread->thread_id << " (" << new_tab << ")";
    ThreadView * tv;

    new_tab = new_tab & !new_window; // only open new tab if not opening new window

    if (new_window) {
      MainWindow * nmw = astroid->open_new_window (false);
      tv = Gtk::manage(new ThreadView (nmw));
      nmw->add_mode (tv);
    } else if (new_tab) {
      tv = Gtk::manage(new ThreadView (main_window));
      main_window->add_mode (tv);
    } else {
      if (!thread_view_loaded) {
        LOG (debug) << "ti: init paned tv";
        thread_view = Gtk::manage(new ThreadView (main_window));
        thread_view_loaded = true;
      }

      tv = thread_view;
    }

    tv->load_thread (thread);
    tv->show ();

    if (!new_tab && !thread_view_visible && !new_window) {
      add_pane (1, *tv);
      thread_view_visible = true;
    }

    // grab modal
    if (!new_tab && !new_window) {
      current = 1;
      grab_modal ();
    }

  }

  void ThreadIndex::pre_close () {
# ifndef DISABLE_PLUGINS
    plugins->deactivate ();
    delete plugins;
# endif
  }

  ThreadIndex::~ThreadIndex () {
    LOG (debug) << "ti: deconstruct.";

    if (thread_view_loaded) {
      delete thread_view; // apparently not done by Gtk::manage
    }
  }
}

