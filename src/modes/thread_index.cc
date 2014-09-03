# include <iostream>
# include <string>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "astroid.hh"
# include "log.hh"
# include "db.hh"
# include "paned_mode.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"
# include "thread_view.hh"
# include "main_window.hh"

using namespace std;

namespace Astroid {
  ThreadIndex::ThreadIndex (MainWindow *mw, ustring _query) : query_string(_query){

    main_window = mw;

    set_orientation (Gtk::Orientation::ORIENTATION_VERTICAL);
    tab_widget = Gtk::manage(new Gtk::Label (query_string));
    tab_widget->set_can_focus (false);

    /* set up treeview */
    list_store = Glib::RefPtr<ThreadIndexListStore>(new ThreadIndexListStore ());
    list_view  = Gtk::manage(new ThreadIndexListView (this, list_store));
    scroll     = Gtk::manage(new ThreadIndexScrolled (list_store, list_view));

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

    log  << info;
    log  << "ti, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_threads (query) << " threads." << endl;

    /* slow */
    threads = notmuch_query_search_threads (query);
    float diff = (clock () - start) * 1000.0 / CLOCKS_PER_SEC;

    log  << "ti: query time: " << diff << " ms." << endl;
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

    list_store->clear ();

    close_query ();
    if (!checked) db->reopen ();
    setup_query ();

    current_thread = 0;

    load_more_threads (all, count, true);

    /* select old */
    Gtk::TreeIter iter;
    while (iter = list_store->get_iter (path), !(iter)) {
      if (!path.prev ()) {
        path = Gtk::TreePath ("0");
        break;
      }
    }
    list_view->set_cursor (path);

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
    log << debug << "ti: key press" << endl;
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

      case GDK_KEY_P:
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


    }

    return false;
  }

  void ThreadIndex::open_thread (refptr<NotmuchThread> thread, bool new_tab) {
    log << debug << "ti: open thread: " << thread->thread_id << " (" << new_tab << ")" << endl;
    ThreadView * tv;

    if (new_tab) {
      tv = Gtk::manage(new ThreadView (main_window));
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

    if (!new_tab && !thread_view_visible) {
      add_pane (1, *tv);
      thread_view_visible = true;
    } else if (new_tab) {
      main_window->add_mode (tv);
    }

    // grab modal
    if (!new_tab) {
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

