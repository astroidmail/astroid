# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestOpenError
# include <boost/test/unit_test.hpp>
# include <boost/filesystem.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"
using namespace std;

using namespace boost::filesystem;

BOOST_AUTO_TEST_SUITE(Reading)

  BOOST_AUTO_TEST_CASE(open_error)
  {
    setup ();

    string fname = "test/mail/test_mail/this-one-should-not-exist.eml";

    BOOST_CHECK (!exists(fname));

    BOOST_CHECK_THROW(Message m (fname), message_error);

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

