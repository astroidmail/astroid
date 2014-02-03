# include <iostream>
# include <string>

# include <gtkmm.h>
# include <gtkmm/label.h>

# include <notmuch.h>

# include "gulp.hh"
# include "db.hh"
# include "thread_index.hh"

using namespace std;

namespace Gulp {
  ThreadIndex::ThreadIndex (string _query) : query_string(_query){

    tab_widget = new Gtk::Label (query_string);

    /* set up notmuch query */
    query =  notmuch_query_create (gulp->db->nm_db, query_string.c_str ());

    cout << "index, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_messages (query) << " messages." << endl;

  }

  ThreadIndex::~ThreadIndex () {

  }
}

