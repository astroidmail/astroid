# pragma once

# include <thread>
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

      bool run = false;
      void loader ();

      std::thread loader_thread;

  };
}

