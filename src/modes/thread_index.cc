# include <iostream>
# include <string>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "astroid.hh"
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
    tab_widget = new Gtk::Label (query_string);
    tab_widget->set_can_focus (false);

    /* set up treeview */
    list_store = Glib::RefPtr<ThreadIndexListStore>(new ThreadIndexListStore ());
    list_view  = new ThreadIndexListView (this, list_store);
    scroll     = new ThreadIndexScrolled (list_store, list_view);

    add_pane (0, *scroll);

    show_all ();

    /* set up notmuch query */
    db = new Db (Db::DbMode::DATABASE_READ_ONLY);
    lock_guard<Db> grd (*db);

    query =  notmuch_query_create (db->nm_db, query_string.c_str ());

    cout << "ti, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_messages (query) << " messages." << endl;

    threads = notmuch_query_search_threads (query);

    /* add threads to model */
    load_more_threads ();

    /* select first */
    list_view->set_cursor (Gtk::TreePath("0"));
  }


  void ThreadIndex::load_more_threads (bool all) {
    notmuch_thread_t  * thread;

    cout << "ti: load more (all: " << all << ") threads.." << endl;
    lock_guard<Db> grd (*db);

    int i = 0;

    for (;
         notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {

      thread = notmuch_threads_get (threads);

      NotmuchThread *t = new NotmuchThread (thread);

      auto iter = list_store->append ();
      Gtk::ListStore::Row row = *iter;

      row[list_store->columns.newest_date] = t->newest_date;
      row[list_store->columns.thread_id]   = t->thread_id;
      row[list_store->columns.thread]      = Glib::RefPtr<NotmuchThread>(t);

      i++;

      if (!all) {
        if (i >= initial_max_threads) {
          break;
        }
      }
    }

    cout << "ti: loaded " << i << " threads." << endl;
  }

  bool ThreadIndex::on_key_press_event (GdkEventKey *event) {
    cout << "ti: key press" << endl;
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
    cout << "ti: open thread: " << thread->thread_id << " (" << new_tab << ")" << endl;
    ThreadView * tv;

    if (new_tab) {
      tv = new ThreadView (main_window);
    } else {
      if (!thread_view_loaded) {
        cout << "ti: init paned tv" << endl;
        thread_view = new ThreadView (main_window);
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
    cout << "ti: deconstruct." << endl;
    notmuch_threads_destroy (threads);
    notmuch_query_destroy (query);

    delete db;

    if (thread_view_loaded) {
      delete thread_view; // apparently not done by Gtk::manage
    }
  }
}

