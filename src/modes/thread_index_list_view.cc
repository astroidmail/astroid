# include "../db.hh"
# include "thread_index_list_view.hh"

using namespace std;

namespace Gulp {
  /* list store */
  ThreadIndexListStore::ThreadIndexListStoreColumnRecord::ThreadIndexListStoreColumnRecord () {
    add (thread_id);
    add (thread);
  }

  ThreadIndexListStore::ThreadIndexListStore () {
    set_column_types (columns);
  }


  /* list view */
  ThreadIndexListView::ThreadIndexListView (Glib::RefPtr<ThreadIndexListStore> store) {
    list_store = store;

    set_model (list_store);

    append_column ("Thread IDs", list_store->columns.thread_id);
  }
}

