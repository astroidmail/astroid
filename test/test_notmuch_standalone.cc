# include <iostream>
# include <boost/filesystem.hpp>

# include <notmuch.h>

/* there was a bit of a round-dance of with the _st versions of these returning
 * to the old name, but with different signature */
# if (LIBNOTMUCH_MAJOR_VERSION < 5)
# define notmuch_query_search_threads(x,y) notmuch_query_search_threads_st(x,y)
# define notmuch_query_count_threads(x,y) notmuch_query_count_threads_st(x,y)
# define notmuch_query_search_messages(x,y) notmuch_query_search_messages_st(x,y)
# define notmuch_query_count_messages(x,y) notmuch_query_count_messages_st(x,y)
# endif

namespace bfs = boost::filesystem;
using std::cout;
using std::endl;

int main () {
  bfs::path path_db = bfs::absolute (bfs::path("./test/mail/test_mail"));

  notmuch_database_t * nm_db;

  notmuch_status_t s =
    notmuch_database_open (
      path_db.c_str(),
      notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
      &nm_db);


  cout << "db: running test query.." << endl;
  notmuch_query_t * q = notmuch_query_create (nm_db, "*");

  unsigned int c;
  notmuch_status_t st = notmuch_query_count_threads (q, &c); // destructive
  notmuch_query_destroy (q);
  q = notmuch_query_create (nm_db, "*");

  cout << "query: " << notmuch_query_get_query_string (q) << ", approx: "
       << c << " threads." << endl;

  notmuch_threads_t * threads;
  notmuch_thread_t  * thread;
  st = notmuch_query_search_threads (q, &threads);

  std::string thread_id;

  int i = 0;

  for (; notmuch_threads_valid (threads);
         notmuch_threads_move_to_next (threads)) {
    thread = notmuch_threads_get (threads);
    i++;

    if (i == 3)
      thread_id = notmuch_thread_get_thread_id (thread);

    notmuch_thread_destroy (thread);

    if (i == 3) break;
  }

  cout << "thread id to change: " << thread_id << ", thread no: " << i << endl;
  notmuch_query_destroy (q);

  /* restart query */
  cout << "restarting query.." << endl;
  q = notmuch_query_create (nm_db, "*");
  st = notmuch_query_search_threads (q, &threads);

  i = 0;
  int stop = 2;

  cout << "moving to thread: " << stop << endl;
  for ( ; notmuch_threads_valid (threads);
          notmuch_threads_move_to_next (threads))
  {
    thread = notmuch_threads_get (threads);
    notmuch_thread_get_thread_id (thread);
    i++;

    cout << "tags: ";

    /* get tags */
    notmuch_tags_t *tags;
    const char *tag;

    for (tags = notmuch_thread_get_tags (thread);
         notmuch_tags_valid (tags);
         notmuch_tags_move_to_next (tags))
    {
        tag = notmuch_tags_get (tags);
        cout << tag << " ";
    }
    cout << endl;

    notmuch_thread_destroy (thread);

    if (i == stop) break;
  }

  /* now open a new db instance, modify the already loaded thread and
   * continue loading the original query */
  notmuch_database_t * nm_db2;

  s = notmuch_database_open (
      path_db.c_str(),
      notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_WRITE,
      &nm_db2);


  char qry_s[256];
  sprintf (qry_s, "thread:%s", thread_id.c_str ());
  notmuch_query_t * q2 = notmuch_query_create (nm_db2, qry_s);
  notmuch_threads_t * ts2;
  notmuch_thread_t  * t2;

  st = notmuch_query_search_threads (q2, &ts2);

  for ( ; notmuch_threads_valid (ts2);
          notmuch_threads_move_to_next (ts2))
  {
    t2 = notmuch_threads_get (ts2);
    std::string thread_id = notmuch_thread_get_thread_id (t2);


    /* remove unread tag */
    notmuch_messages_t * ms = notmuch_thread_get_messages (t2);
    notmuch_message_t  * m;

    for (; notmuch_messages_valid (ms); notmuch_messages_move_to_next (ms)) {
      m = notmuch_messages_get (ms);

      st = notmuch_message_remove_tag (m, "unread");

      notmuch_message_destroy (m);
    }

    notmuch_messages_destroy (ms);


    notmuch_thread_destroy (t2);
    break;
  }

  notmuch_query_destroy (q2);
  notmuch_database_close (nm_db2);

  /* re-add unread tag */
  s = notmuch_database_open (
      path_db.c_str(),
      notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_WRITE,
      &nm_db2);



  q2 = notmuch_query_create (nm_db2, qry_s);

  st = notmuch_query_search_threads (q2, &ts2);

  for ( ; notmuch_threads_valid (ts2);
          notmuch_threads_move_to_next (ts2))
  {
    t2 = notmuch_threads_get (ts2);
    std::string thread_id = notmuch_thread_get_thread_id (t2);


    /* remove unread tag */
    notmuch_messages_t * ms = notmuch_thread_get_messages (t2);
    notmuch_message_t  * m;

    for (; notmuch_messages_valid (ms); notmuch_messages_move_to_next (ms)) {
      m = notmuch_messages_get (ms);

      st = notmuch_message_add_tag (m, "unread");

      notmuch_message_destroy (m);
    }

    notmuch_messages_destroy (ms);


    notmuch_thread_destroy (t2);
    break;
  }

  notmuch_query_destroy (q2);
  notmuch_database_close (nm_db2);

  /* continue loading */
  cout << "continue loading.." << endl;
  for ( ; notmuch_threads_valid (threads);
          notmuch_threads_move_to_next (threads))
  {
    if (threads == NULL) {
      cout << "threads == NULL" << endl;
    } else {
      cout << "threads != NULL" << endl;
    }
    thread = notmuch_threads_get (threads);
    cout << "loading: " << i;
    std::string tid = notmuch_thread_get_thread_id (thread);
    cout << ": " << tid << endl;

    /* get tags */
    notmuch_tags_t *tags;
    const char *tag;

    cout << "tags: ";
    for (tags = notmuch_thread_get_tags (thread);
         notmuch_tags_valid (tags);
         notmuch_tags_move_to_next (tags))
    {
        tag = notmuch_tags_get (tags);
        cout << tag << " ";
    }
    cout << endl;

    i++;
    notmuch_thread_destroy (thread);
  }

  notmuch_database_close (nm_db);
  return 0;
}

