# include "astroid.hh"
# include "db.hh"
# include "log.hh"

# include "query_loader.hh"
# include "thread_index_list_view.hh"
# include "config.hh"

# include <thread>
# include <queue>
# include <mutex>
# include <notmuch.h>

using std::endl;

namespace Astroid {
  QueryLoader::QueryLoader () {
    ustring sort_order = astroid->config ().get<std::string> ("thread_index.sort_order");
    if (sort_order == "newest") {
      sort = NOTMUCH_SORT_NEWEST_FIRST;
    } else if (sort_order == "oldest") {
      sort = NOTMUCH_SORT_OLDEST_FIRST;
    } else if (sort_order == "messageid") {
      sort = NOTMUCH_SORT_MESSAGE_ID;
    } else if (sort_order == "unsorted") {
      sort = NOTMUCH_SORT_UNSORTED;
    } else {
      log << error << "ti: unknown sort order, must be 'newest', 'oldest', 'messageid' or 'unsorted': " << sort_order << ", using 'newest'." << endl;
      sort = NOTMUCH_SORT_NEWEST_FIRST;
    }

    loaded_threads = 0;
    total_messages = 0;
    unread_messages = 0;

    queue_has_data.connect (std::bind (&QueryLoader::to_list_adder, this));
  }

  QueryLoader::~QueryLoader () {
    in_destructor = true;
    stop ();
  }

  void QueryLoader::start (ustring q) {
    query = q;
    run = true;
    loader_thread = std::thread (&QueryLoader::loader, this);
  }

  void QueryLoader::stop () {
    run = false;
    loader_thread.join ();
  }

  void QueryLoader::reload () {
    stop ();
    list_store->clear ();
    start (query);
  }

  void QueryLoader::refine_query (ustring q) {
    query = q;
    reload ();
  }

  void QueryLoader::refresh_stats (Db * db) {
    log << debug << "ql: refresh stats.." << endl;

    notmuch_query_t * query_t =  notmuch_query_create (db->nm_db, query.c_str ());
    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (query_t, t.c_str());
    }
    notmuch_query_set_omit_excluded (query_t, NOTMUCH_EXCLUDE_TRUE);
    /* st = */ notmuch_query_count_messages_st (query_t, &total_messages); // destructive
    notmuch_query_destroy (query_t);

    ustring unread_q_s = "(" + query + ") AND tag:unread";
    notmuch_query_t * unread_q = notmuch_query_create (db->nm_db, unread_q_s.c_str());
    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (unread_q, t.c_str());
    }
    notmuch_query_set_omit_excluded (unread_q, NOTMUCH_EXCLUDE_TRUE);
    /* st = */ notmuch_query_count_messages_st (unread_q, &unread_messages); // destructive
    notmuch_query_destroy (unread_q);

    if (!in_destructor) stats_ready.emit ();
  }

  void QueryLoader::loader () {
    time_t t0 = clock ();

    log << debug << "ql: load threads for query: " << query << endl;


    Db db (Db::DATABASE_READ_ONLY);

    /* set up query */
    notmuch_query_t * nmquery;
    notmuch_threads_t * threads;

    nmquery = notmuch_query_create (db.nm_db, query.c_str ());
    for (ustring & t : db.excluded_tags) {
      notmuch_query_add_tag_exclude (nmquery, t.c_str());
    }

    notmuch_query_set_omit_excluded (nmquery, NOTMUCH_EXCLUDE_TRUE);
    notmuch_query_set_sort (nmquery, sort);

    /* slow */
    /* notmuch_status_t st = */ notmuch_query_search_threads_st (nmquery, &threads);
    float diff = (clock () - t0) * 1000.0 / CLOCKS_PER_SEC;

    log << debug << "ql: query time: " << diff << " ms." << endl;

    loaded_threads = 0; // incremented in list_adder
    int i = 0;

    for (;
         run && notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {

      notmuch_thread_t  * thread;
      thread = notmuch_threads_get (threads);

      if (thread == NULL) {
        log << error << "ql: error: could not get thread." << endl;
        throw database_error ("ql: could not get thread (is NULL)");
      }

      NotmuchThread *t = new NotmuchThread (thread);

      notmuch_thread_destroy (thread);

      std::unique_lock<std::mutex> lk (to_list_m);

      to_list_store.push (refptr<NotmuchThread>(t));

      lk.unlock ();

      i++;

      if ((i % 100) == 0) {
        if (!in_destructor)
          queue_has_data.emit ();
      }
    }

    /* closing query */
    notmuch_threads_destroy (threads);
    notmuch_query_destroy (nmquery);

    refresh_stats (&db);

    log << info << "ql: loaded " << i << " threads in " << ((clock()-t0) * 1000.0 / CLOCKS_PER_SEC) << " ms." << endl;

    if (!run) {
      log << warn << "ql: stopped before finishing." << endl;
    }

    // catch any remaining entries
    if (!in_destructor)
      queue_has_data.emit ();

    run = false;
  }

  void QueryLoader::to_list_adder () {
    std::lock_guard<std::mutex> lk (to_list_m);

    while (!to_list_store.empty ()) {
      refptr<NotmuchThread> t = to_list_store.front ();
      to_list_store.pop ();

      auto iter = list_store->append ();
      Gtk::ListStore::Row row = *iter;

      row[list_store->columns.newest_date] = t->newest_date;
      row[list_store->columns.oldest_date] = t->oldest_date;
      row[list_store->columns.thread_id]   = t->thread_id;
      row[list_store->columns.thread]      = t;

      if (loaded_threads == 0) {
        if (!in_destructor)
          first_thread_ready.emit ();
      }

      loaded_threads++;

      if ((loaded_threads % 100) == 0) {
        log << debug << "ql: loaded " << loaded_threads << " threads." << endl;
      }
    }
  }
}

