# include <iostream>
# include <string>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "astroid.hh"
# include "log.hh"
# include "db.hh"
# include "modes/paned_mode.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "modes/thread_view.hh"
# include "main_window.hh"

using namespace std;

namespace Astroid {
  ThreadIndex::ThreadIndex (MainWindow *mw, ustring _query) : PanedMode(mw), query_string(_query) {

    set_orientation (Gtk::Orientation::ORIENTATION_VERTICAL);
    set_label (query_string);

    /* set up treeview */
    list_store = Glib::RefPtr<ThreadIndexListStore>(new ThreadIndexListStore ());
    list_view  = Gtk::manage(new ThreadIndexListView (this, list_store));
    scroll     = Gtk::manage(new ThreadIndexScrolled (main_window, list_store, list_view));

    add_pane (0, *scroll);

    show_all ();

    /* load threads */
    db = new Db (Db::DbMode::DATABASE_READ_ONLY);
    setup_query ();
    load_more_threads ();

    /* select first */
    list_view->set_cursor (Gtk::TreePath("0"));
  }

  void ThreadIndex::setup_query () {
    lock_guard <Db> grd (*db);

    /* set up notmuch query */
    time_t start = clock ();
    query =  notmuch_query_create (db->nm_db, query_string.c_str ());

    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (query, t.c_str());
    }
    notmuch_query_set_omit_excluded (query, NOTMUCH_EXCLUDE_TRUE);

    /* notmuch_query_count_threads is destructive.
     *
    log  << info;
    log  << "ti, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_threads (query) << " threads and " << notmuch_query_count_messages (query) << " messages matching."  << endl;

    */

    /* slow */
    threads = notmuch_query_search_threads (query);
    float diff = (clock () - start) * 1000.0 / CLOCKS_PER_SEC;

    refresh_stats (db);
    float diffstat = (clock () - start) * 1000.0 / CLOCKS_PER_SEC;

    log << debug << "ti: query, total: " << total_messages << ", unread: " << unread_messages << endl;
    log << debug << "ti: query time: " << diff << " ms (with stat: " << diffstat << " ms." << endl;

  }

  void ThreadIndex::refresh_stats (Db * dbs) {
    /* stats */
    log << debug << "ti: refresh stats." << endl;
    notmuch_query_t * query_t =  notmuch_query_create (dbs->nm_db, query_string.c_str ());
    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (query_t, t.c_str());
    }
    notmuch_query_set_omit_excluded (query_t, NOTMUCH_EXCLUDE_TRUE);
    total_messages = notmuch_query_count_messages (query_t); // destructive
    notmuch_query_destroy (query_t);

    ustring unread_q_s = "(" + query_string + ") AND tag:unread";
    notmuch_query_t * unread_q = notmuch_query_create (dbs->nm_db, unread_q_s.c_str());
    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (unread_q, t.c_str());
    }
    notmuch_query_set_omit_excluded (unread_q, NOTMUCH_EXCLUDE_TRUE);
    unread_messages = notmuch_query_count_messages (unread_q); // destructive
    notmuch_query_destroy (unread_q);

    set_label (ustring::compose ("%1 (%2/%3)", query_string, unread_messages, total_messages));
  }

  void ThreadIndex::close_query () {
    notmuch_threads_destroy (threads);
    notmuch_query_destroy (query);
  }


  void ThreadIndex::refresh (bool all, int count, bool checked) {
    log << debug << "ti: refresh." << endl;

    time_t t0 = clock ();

    /* get current path */
    Gtk::TreePath path;
    Gtk::TreeViewColumn * c;
    list_view->get_cursor (path, c);

    auto adj          = list_view->get_vadjustment ();
    double scroll_val = adj->get_value();

    list_store->clear ();

    close_query ();
    if (!checked) db->reopen ();
    setup_query ();

    current_thread = 0;

    load_more_threads (all, count, true);

    /* select old */
    if (current_thread > 0) {
      if (!path) {
        path = Gtk::TreePath ("0");
      } else {
        Gtk::TreeIter iter;
        while (iter = list_store->get_iter (path), !(iter)) {
          if (!path.prev ()) {
            path = Gtk::TreePath ("0");
            break;
          }
        }
      }
      list_view->set_cursor (path);

      adj->set_value (scroll_val); // probably need to do this after everything has been drawn
    }

    log << debug << "ti: refreshed in " << ((clock() - t0) * 1000.0 / CLOCKS_PER_SEC) << " ms." << endl;
  }

  void ThreadIndex::load_more_threads (bool all, int count, bool checked) {
    time_t t0 = clock ();

    log << debug << "ti: load more (all: " << all << ") threads.." << endl;
    lock_guard<Db> grd (*db);

    if (count < 0) count = thread_load_step;

    if (reopen_tries > 1) {
      log << error << "ti: could not reopen db." << endl;

      throw database_error ("ti: could not reopen db.");
    }

    if (!checked) {
      if (db->check_reopen (true)) {
        reopen_tries++;
        refresh (all, current_thread + count, true);
        return;
      }
    }

    reopen_tries = 0;

    int i = 0;

    for (;
         notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {

      notmuch_thread_t  * thread;
      thread = notmuch_threads_get (threads);

      if (thread == NULL) {
        log << error << "ti: error: could not get thread." << endl;
        throw database_error ("ti: could not get thread (is NULL)");
      }

      /* test for revision discarded */
      const char * ti = notmuch_thread_get_thread_id (thread);
      if (ti == NULL) {
        log << error << "ti: revision discarded, trying to reopen." << endl;
        reopen_tries++;
        refresh (all, current_thread + count, false);
        return;
      }


      NotmuchThread *t = new NotmuchThread (thread);

      notmuch_thread_destroy (thread);

      auto iter = list_store->append ();
      Gtk::ListStore::Row row = *iter;

      row[list_store->columns.newest_date] = t->newest_date;
      row[list_store->columns.thread_id]   = t->thread_id;
      row[list_store->columns.thread]      = Glib::RefPtr<NotmuchThread>(t);

      i++;
      current_thread++;

      if ((i % 100) == 0) {
        log << debug << "ti: loaded " << i << " threads." << endl;
      }

      if (!all) {
        if (i >= count) {
          break;
        }
      }
    }

    log << info << "ti: loaded " << i << " threads in " << ((clock()-t0) * 1000.0 / CLOCKS_PER_SEC) << " ms." << endl;
  }

  bool ThreadIndex::on_key_press_event (GdkEventKey *event) {
    switch (event->keyval) {
      case GDK_KEY_x:
        {
          if (current == 1) {
            if (thread_view_loaded && thread_view_visible) {
              /* hide thread view */
              del_pane (1);
              thread_view_visible = false;

              return true;
            }
          }
        }
        break;

      case GDK_KEY_dollar:
        {
          refresh (false, max(thread_load_step, current_thread), false);
          return true;
        }

      /* toggle between panes */
      case GDK_KEY_Tab:
        if (packed == 2) {
          release_modal ();
          current = (current == 0 ? 1 : 0);
          grab_modal ();
        }
        return true;

      case GDK_KEY_v:
        {
          main_window->enable_command (CommandBar::CommandMode::Search,
              query_string,
              [&](ustring new_query) {

                query_string = new_query;
                set_label (query_string);
                list_store->clear ();
                close_query ();
                setup_query ();
                load_more_threads ();

                /* select first */
                list_view->set_cursor (Gtk::TreePath("0"));

              });

          return true;
        }

    }

    return false;
  }

  ModeHelpInfo * ThreadIndex::key_help () {
    ModeHelpInfo * m = new ModeHelpInfo ();

    m->parent   = PanedMode::key_help ();
    m->toplevel = false;
    m->title    = "Thread Index";

    m->keys = {
      { "x", "Close thread view pane if open" },
      { "Tab", "Swap focus to other pane if open" },
      { "$", "Refresh query" },
      { "v", "Refine query" }
    };

    ModeHelpInfo * lm = list_view->key_help ();
    lm->parent = m;

    return lm;
  }

  void ThreadIndex::open_thread (refptr<NotmuchThread> thread, bool new_tab, bool new_window) {
    log << debug << "ti: open thread: " << thread->thread_id << " (" << new_tab << ")" << endl;
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
        log << debug << "ti: init paned tv" << endl;
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

  ThreadIndex::~ThreadIndex () {
    log << debug << "ti: deconstruct." << endl;
    close_query ();
    delete db;

    if (thread_view_loaded) {
      delete thread_view; // apparently not done by Gtk::manage
    }
  }
}

