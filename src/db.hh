# pragma once

# include <vector>

# include <time.h>

# include <glibmm.h>
# include <notmuch.h>

# include "astroid.hh"
# include "config.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  /* the notmuch thread object should get by on the db only */
  class NotmuchThread : public Glib::Object {
    public:
      NotmuchThread (notmuch_thread_t *);
      NotmuchThread (ustring);
      ~NotmuchThread ();

      string thread_id;

      char * subject_chr;
      ustring subject;
      time_t  newest_date;
      bool    unread;
      bool    attachment;
      bool    flagged;
      int     total_messages;
      vector<ustring> authors;
      vector<ustring> tags;

      void refresh ();

      bool remove_tag (ustring);
      bool add_tag (ustring);
      ustring sanitize_tag (ustring);
      bool check_tag (ustring);

    private:
      int     check_total_messages ();
      vector<ustring> get_authors ();
      vector<ustring> get_tags ();

      /* activate valid db objects */
      void activate ();
      void deactivate ();

      int ref = 0;
      notmuch_query_t *   query;
      notmuch_threads_t * nm_threads;
      notmuch_thread_t *  nm_thread;
  };

  class Db {
    public:
      Db ();
      ~Db ();

      ptree config;

      notmuch_database_t * nm_db;

      vector<ustring> tags;

      void load_tags ();
      void test_query ();

      vector<ustring> sent_tags = { "sent" };
      void add_sent_message (ustring);
  };
}

