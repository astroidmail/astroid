# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestNotmuch
# include <boost/test/unit_test.hpp>
# include <boost/filesystem.hpp>

# include <iostream>

# include "test_common.hh"

# include <notmuch.h>
# include "db.hh"

namespace bfs = boost::filesystem;
using std::cout;
using std::endl;

BOOST_AUTO_TEST_SUITE(Notmuch)

  BOOST_AUTO_TEST_CASE(read_all_threads)
  {

    bfs::path path_db = bfs::absolute (bfs::path("./test/mail/test_mail"));

    notmuch_database_t * nm_db;

    notmuch_status_t s =
      notmuch_database_open (
        path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
        &nm_db);


    BOOST_CHECK (s == NOTMUCH_STATUS_SUCCESS);

    /* read all messages */
    cout << "db: running test query.." << endl;
    notmuch_query_t * q = notmuch_query_create (nm_db, "*");

    unsigned int c;
    notmuch_status_t st = notmuch_query_count_messages (q, &c); // desctrutive
    notmuch_query_destroy (q);
    q = notmuch_query_create (nm_db, "*");

    BOOST_CHECK (st == NOTMUCH_STATUS_SUCCESS);

    cout << "query: " << notmuch_query_get_query_string (q) << ", approx: "
         << c << " messages." << endl;

    notmuch_messages_t * messages;
    notmuch_message_t  * message;

    for (st = notmuch_query_search_messages (q, &messages);

         (st == NOTMUCH_STATUS_SUCCESS) &&
         notmuch_messages_valid (messages);

         notmuch_messages_move_to_next (messages))
    {
      message = notmuch_messages_get (messages);

      cout << "thread:" << notmuch_message_get_thread_id (message) << ", message: " << notmuch_message_get_header (message, "Subject") << endl;

      notmuch_message_destroy (message);
    }


    notmuch_database_close (nm_db);
  }

  /* BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES (notmuch_threads_move_to_next_fail, 1) */
  BOOST_AUTO_TEST_CASE(notmuch_threads_move_to_next_fail)
  {

    bfs::path path_db = bfs::absolute (bfs::path("./test/mail/test_mail"));

    notmuch_database_t * nm_db;

    notmuch_status_t s =
      notmuch_database_open (
        path_db.c_str(),
        notmuch_database_mode_t::NOTMUCH_DATABASE_MODE_READ_ONLY,
        &nm_db);


    BOOST_CHECK (s == NOTMUCH_STATUS_SUCCESS);

    cout << "db: running test query.." << endl;
    notmuch_query_t * q = notmuch_query_create (nm_db, "*");

    unsigned int c;
    notmuch_status_t st = notmuch_query_count_threads (q, &c); // destructive
    notmuch_query_destroy (q);
    q = notmuch_query_create (nm_db, "*");

    BOOST_CHECK (st == NOTMUCH_STATUS_SUCCESS);

    cout << "query: " << notmuch_query_get_query_string (q) << ", approx: "
         << c << " threads." << endl;

    notmuch_threads_t * threads;
    notmuch_thread_t  * thread;
    st = notmuch_query_search_threads (q, &threads);

    std::string thread_id_of_first;
    int i = 0;
    int stop = 3;

    for (; notmuch_threads_valid (threads);
           notmuch_threads_move_to_next (threads)) {
      thread = notmuch_threads_get (threads);
      thread_id_of_first = notmuch_thread_get_thread_id (thread);
      notmuch_thread_destroy (thread);
      i++;

      if (i == stop) break;
    }

    cout << "thread id of first thread: " << thread_id_of_first << endl;
    notmuch_query_destroy (q);

    /* restart query */
    cout << "restarting query.." << endl;
    q = notmuch_query_create (nm_db, "*");
    st = notmuch_query_search_threads (q, &threads);

    i = 0;

    for ( ; notmuch_threads_valid (threads);
            notmuch_threads_move_to_next (threads))
    {
      i++;
      cout << "move to next: " << i << endl;
      if (i == stop) break;
    }
    notmuch_threads_move_to_next (threads);

    for ( ; notmuch_threads_valid (threads);
            notmuch_threads_move_to_next (threads))
    {
      thread = notmuch_threads_get (threads);
      std::string thread_id = notmuch_thread_get_thread_id (thread);
      i++;

      BOOST_CHECK_MESSAGE (thread_id != thread_id_of_first, "thread id is equal to " << stop << " thread, we are on thread: " << i);

      notmuch_thread_destroy (thread);
    }

    notmuch_database_close (nm_db);
  }

BOOST_AUTO_TEST_SUITE_END()

