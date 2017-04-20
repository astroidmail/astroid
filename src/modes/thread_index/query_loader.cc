# include "astroid.hh"
# include "db.hh"

# include "query_loader.hh"
# include "thread_index_list_view.hh"
# include "config.hh"
# include "actions/action_manager.hh"

# include <thread>
# include <queue>
# include <mutex>
# include <functional>

# include <notmuch.h>

using std::endl;

namespace Astroid {
  int QueryLoader::nextid = 0;

  QueryLoader::QueryLoader () {
    id = nextid++;

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
      LOG (error) << "ti: unknown sort order, must be 'newest', 'oldest', 'messageid' or 'unsorted': " << sort_order << ", using 'newest'.";
      sort = NOTMUCH_SORT_NEWEST_FIRST;
    }

    loaded_threads = 0;
    total_messages = 0;
    unread_messages = 0;
    run = false;

    queue_has_data.connect (
        sigc::mem_fun (this, &QueryLoader::to_list_adder));

    deferred_threads_d.connect (
        sigc::mem_fun (this, &QueryLoader::update_deferred_changed_threads));

    make_stats.connect (
        sigc::mem_fun (this, &QueryLoader::refresh_stats));

    astroid->actions->signal_thread_changed ().connect (
        sigc::mem_fun (this, &QueryLoader::on_thread_changed));

    astroid->actions->signal_refreshed ().connect (
        sigc::mem_fun (this, &QueryLoader::on_refreshed));
  }

  QueryLoader::~QueryLoader () {
    LOG (debug) << "ql: destruct.";
    in_destructor = true;
    stop ();
  }

  void QueryLoader::start (ustring q) {
    std::lock_guard<std::mutex> lk (loader_m);
    query = q;
    run = true;
    loader_thread = std::thread (&QueryLoader::loader, this);
  }

  void QueryLoader::stop () {
    if (run) {
      LOG (info) << "ql (" << id << "): stopping loader...";
    }

    run = false;
    loader_thread.join ();
  }

  void QueryLoader::reload () {
    stop ();
    std::lock_guard<std::mutex> lk (to_list_m);
    list_store->clear ();

    while (!to_list_store.empty ())
      to_list_store.pop ();

    start (query);
  }

  void QueryLoader::refine_query (ustring q) {
    query = q;
    reload ();
  }

  void QueryLoader::refresh_stats () {
    Db db (Db::DATABASE_READ_ONLY);
    refresh_stats_db (&db);
  }

  void QueryLoader::refresh_stats_db (Db * db) {
    LOG (debug) << "ql: refresh stats..";

    notmuch_status_t st = NOTMUCH_STATUS_SUCCESS;

    notmuch_query_t * query_t =  notmuch_query_create (db->nm_db, query.c_str ());
    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (query_t, t.c_str());
    }
    notmuch_query_set_omit_excluded (query_t, NOTMUCH_EXCLUDE_TRUE);
    st = notmuch_query_count_messages (query_t, &total_messages); // destructive
    if (st != NOTMUCH_STATUS_SUCCESS) total_messages = 0;
    notmuch_query_destroy (query_t);

    ustring unread_q_s = "(" + query + ") AND tag:unread";
    notmuch_query_t * unread_q = notmuch_query_create (db->nm_db, unread_q_s.c_str());
    for (ustring & t : db->excluded_tags) {
      notmuch_query_add_tag_exclude (unread_q, t.c_str());
    }
    notmuch_query_set_omit_excluded (unread_q, NOTMUCH_EXCLUDE_TRUE);
    st = notmuch_query_count_messages (unread_q, &unread_messages); // destructive
    if (st != NOTMUCH_STATUS_SUCCESS) unread_messages = 0;
    notmuch_query_destroy (unread_q);

    if (!in_destructor) stats_ready.emit ();

    waiting_stats = false;
  }

  void QueryLoader::loader () {
    std::lock_guard<std::mutex> loader_lk (loader_m);

    Db db (Db::DATABASE_READ_ONLY);
    refresh_stats_db (&db);

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
    notmuch_status_t st = NOTMUCH_STATUS_SUCCESS;
    st = notmuch_query_search_threads (nmquery, &threads);

    if (st != NOTMUCH_STATUS_SUCCESS) {
      LOG (error) << "ql: could not get threads for query: " << query;
      run = false;
    }

    loaded_threads = 0; // incremented in list_adder
    int i = 0;

    for (;
         run && notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {

      notmuch_thread_t  * thread;
      thread = notmuch_threads_get (threads);

      if (thread == NULL) {
        LOG (error) << "ql: error: could not get thread.";
        throw database_error ("ql: could not get thread (is NULL)");
      }

      NotmuchThread *t = new NotmuchThread (thread);

      notmuch_thread_destroy (thread);

      std::unique_lock<std::mutex> lk (to_list_m);

      to_list_store.push (refptr<NotmuchThread>(t));

      lk.unlock ();

      i++;

      if ((i % 100) == 0) {
        if (run && !in_destructor)
          queue_has_data.emit ();
      }
    }

    /* closing query */
    if (st == NOTMUCH_STATUS_SUCCESS) notmuch_threads_destroy (threads);
    notmuch_query_destroy (nmquery);

    if (!in_destructor)
      stats_ready.emit (); // update loading status

    // catch any remaining entries
    if (!in_destructor)
      queue_has_data.emit ();

    run = false; // on_thread_changed will not check lock

    if (!in_destructor)
      deferred_threads_d.emit ();
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
        LOG (debug) << "ql: loaded " << loaded_threads << " threads.";
        if (!in_destructor && !list_view->filter_txt.empty()) stats_ready.emit ();
      }
    }
  }

  void QueryLoader::update_deferred_changed_threads () {
    /* lock and check for changed threads */
    if (!in_destructor) {
      Db db (Db::DATABASE_READ_ONLY);

      while (!changed_threads.empty ()) {
        ustring tid = changed_threads.front ();
        changed_threads.pop ();
        LOG (debug) << "ql: deferred update of: " << tid;
        on_thread_changed (&db, tid);
      }
    }
  }

  bool QueryLoader::loading () {
    return run;
  }

  /***************
   * signals
   **************/
  void QueryLoader::on_refreshed () {
    if (in_destructor) return;

    LOG (warn) << "ql (" << id << "): got refreshed signal.";
    reload ();
  }

  void QueryLoader::on_thread_changed (Db * db, ustring thread_id) {
    if (in_destructor) return;

    LOG (info) << "ql (" << id << "): " << query << ", got changed thread signal: " << thread_id;

    if (loading ()) {
      LOG (debug) << "ql: still loading, deferring thread_changed to until load is done.";
      changed_threads.push (thread_id);
      return;
    }

    /* we now have three options:
     * - a new thread has been added (unlikely)
     * - a thread has been deleted (kind of likely)
     * - a thread has been updated (most likely)
     *
     * none of them needs to affect the threads that match the query in this
     * list.
     *
     */

    time_t t0 = clock ();

    Gtk::TreePath path;
    Gtk::TreeIter fwditer;

    /* forward iterating is much faster than going backwards:
     * https://developer.gnome.org/gtkmm/3.11/classGtk_1_1TreeIter.html
     */

    bool found = false;
    bool changed = false;
    fwditer = list_store->get_iter ("0");

    Gtk::ListStore::Row row;

    while (fwditer) {

      row = *fwditer;

      if (row[list_store->columns.thread_id] == thread_id) {
        found = true;
        break;
      }

      fwditer++;
    }

    /* test if thread is in the current query */
    bool in_query = db->thread_in_query (query, thread_id);

    if (found) {
      /* thread has either been updated or deleted from current query */
      LOG (debug) << "ql: updated: found thread in: " << ((clock() - t0) * 1000.0 / CLOCKS_PER_SEC) << " ms.";

      if (in_query) {
        /* updated */
        LOG (debug) << "ql: updated";
        refptr<NotmuchThread> thread = row[list_store->columns.thread];
        thread->refresh (db);
        row[list_store->columns.newest_date] = thread->newest_date;
        row[list_store->columns.oldest_date] = thread->oldest_date;

      } else {
        /* deleted */
        LOG (debug) << "ql: deleted";
        path = list_store->get_path (fwditer);
        list_store->erase (fwditer);
      }

      changed = true;

    } else {
      /* thread has possibly been added to the current query */
      LOG (debug) << "ql: updated: did not find thread, time used: " << ((clock() - t0) * 1000.0 / CLOCKS_PER_SEC) << " ms.";
      if (in_query) {
        LOG (debug) << "ql: new thread for query, adding..";

        /* get current cursor path, if we are at first row and the new addition
         * is before we should scroll up. */
        Gtk::TreePath path;
        Gtk::TreeViewColumn *c;
        list_view->get_cursor (path, c);

        auto iter = list_store->prepend ();
        Gtk::ListStore::Row newrow = *iter;

        NotmuchThread * t;

        db->on_thread (thread_id, [&t](notmuch_thread_t *nmt) {

            t = new NotmuchThread (nmt);

          });

        newrow[list_store->columns.newest_date] = t->newest_date;
        newrow[list_store->columns.oldest_date] = t->oldest_date;
        newrow[list_store->columns.thread_id]   = t->thread_id;
        newrow[list_store->columns.thread]      = Glib::RefPtr<NotmuchThread>(t);

        /* check if we should select it (if this is the only item) */
        if (list_store->children().size() == 1) {
          if (!in_destructor)
            first_thread_ready.emit ();
        } else {

          if (path == Gtk::TreePath ("0")) {
            Gtk::TreePath addpath = list_store->get_path (iter);
            if (addpath <= path) {
              list_view->set_cursor (addpath);
            }
          }
        }

        changed = true;
      }
    }

    if (changed && !waiting_stats) {
      waiting_stats = true;
      refresh_stats_db (db); // we should already be running on the gui thread
    }
  }
}

