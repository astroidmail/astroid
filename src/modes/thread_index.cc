# include <iostream>
# include <string>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "../gulp.hh"
# include "../db.hh"
# include "thread_index.hh"
# include "thread_index_list_view.hh"
# include "thread_index_list_cell_renderer.hh"

using namespace std;

namespace Gulp {
  ThreadIndex::ThreadIndex (string _query) : query_string(_query){

    set_orientation (Gtk::Orientation::ORIENTATION_VERTICAL);
    tab_widget = new Gtk::Label (query_string);

    /* set up notmuch query */
    query =  notmuch_query_create (gulp->db->nm_db, query_string.c_str ());

    cout << "index, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_messages (query) << " messages." << endl;


    /* set up treeview */
    list_store = Glib::RefPtr<ThreadIndexListStore>(new ThreadIndexListStore ());
    list_view  = new ThreadIndexListView (list_store);
    pack_start (*list_view, true, true, 5);
    show_all ();

    /* add threads to model */
    notmuch_threads_t * threads;
    notmuch_thread_t  * thread;

    for (threads = notmuch_query_search_threads (query);
         notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {
      thread = notmuch_threads_get (threads);

      NotmuchThread *t = new NotmuchThread (thread);

      auto iter = list_store->append ();
      Gtk::ListStore::Row row = *iter;

      row[list_store->columns.thread_id] = notmuch_thread_get_thread_id (thread);
      row[list_store->columns.thread]    = *t;

    }
  }

  ThreadIndex::~ThreadIndex () {

  }
}

