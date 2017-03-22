# pragma once

# include <thread>
# include <mutex>
# include <queue>
# include <notmuch.h>

# include "proto.hh"
# include "thread_index_list_view.hh"

namespace Astroid {
  class QueryLoader : public sigc::trackable {
    public:
      static int nextid;

      int id;
      QueryLoader ();
      ~QueryLoader ();

      void refine_query (ustring);

      void start (ustring);
      void stop ();
      void reload ();

      unsigned int loaded_threads;
      unsigned int total_messages;
      unsigned int unread_messages;

      void refresh_stats ();
    private:
      void refresh_stats_db (Db *);
    public:

      refptr<ThreadIndexListStore> list_store;
      ThreadIndexListView * list_view;

      notmuch_sort_t sort;
      std::vector<ustring> sort_strings = { "oldest", "newest", "messageid", "unsorted" };

      Glib::Dispatcher first_thread_ready;

      Glib::Dispatcher make_stats;
      bool waiting_stats = false;

      Glib::Dispatcher stats_ready;

      bool loading ();
    private:
      ustring query;

      std::atomic<bool> run;
      bool in_destructor = false;
      void loader ();

      std::thread loader_thread;
      std::mutex  loader_m;

      std::queue<refptr<NotmuchThread>> to_list_store;
      std::mutex to_list_m;

      void to_list_adder ();
      Glib::Dispatcher queue_has_data;

      /* this is a list of threads that got a changed signal
       * while loading */
      Glib::Dispatcher deferred_threads_d;
      void update_deferred_changed_threads ();
      std::queue<ustring> changed_threads;

      /* signal handlers */
      void on_thread_changed (Db *, ustring);
      void on_refreshed ();
  };
}

