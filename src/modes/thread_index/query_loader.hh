# pragma once

# include <thread>
# include <mutex>
# include <queue>
# include <notmuch.h>

# include "proto.hh"
# include "thread_index_list_view.hh"

namespace Astroid {
  class QueryLoader {
    public:
      QueryLoader ();
      ~QueryLoader ();

      void refine_query (ustring);

      void start (ustring);
      void stop ();
      void reload ();

      unsigned int loaded_threads;
      unsigned int total_messages;
      unsigned int unread_messages;

      void refresh_stats (Db *);

      refptr<ThreadIndexListStore> list_store;

      notmuch_sort_t sort;
      std::vector<ustring> sort_strings = { "oldest", "newest", "messageid", "unsorted" };

      Glib::Dispatcher first_thread_ready;
      Glib::Dispatcher stats_ready;

    private:
      ustring query;

      std::atomic<bool> run;
      bool in_destructor = false;
      void loader ();

      std::thread loader_thread;

      std::queue<refptr<NotmuchThread>> to_list_store;
      std::mutex to_list_m;

      void to_list_adder ();
      Glib::Dispatcher queue_has_data;

      /* signal handlers */

      /*
       * if the query is loading, defer these events
       * untill the queue is loaded:
       *
       * to_list_adder () will check this queue when (run == false)
       *
       * (synchornization on this list should not be needed since both
       *  on_thread_changed and to_list_adder are run on the main GUI
       *  thread)
       *
       */
      std::queue<ustring> thread_changed_events_waiting;

      void on_thread_changed (Db *, ustring);
      void on_refreshed ();
  };
}

