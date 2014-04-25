# pragma once

# include <vector>

# include <time.h>

# include <glibmm.h>
# include <notmuch.h>

# include <astroid.hh>
# include "proto.hh"

using namespace std;

namespace Astroid {
  /* the notmuch thread object should get by on the db only */
  class NotmuchThread : public Glib::Object {
    public:
      NotmuchThread (notmuch_thread_t *);

      string thread_id;

      char * subject_chr;
      ustring subject;
      time_t  newest_date;
      bool    unread;
      bool    attachment;
      int     total_messages;
      vector<ustring> authors;
      vector<ustring> tags;

      void    ensure_valid ();

      bool remove_tag (ustring);
      bool add_tag (ustring);

    private:
      int     check_total_messages (notmuch_thread_t *);
      vector<ustring> get_authors (notmuch_thread_t *);
      vector<ustring> get_tags (notmuch_thread_t *);

      notmuch_thread_t * get_nm_thread ();
      void destroy_nm_thread (notmuch_thread_t *);
  };

  class Db {
    public:
      Db ();
      ~Db ();

      notmuch_database_t * nm_db;

      void test_query ();
  };
}

