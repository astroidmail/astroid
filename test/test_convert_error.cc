# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestConvertError
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"
using namespace std;


BOOST_AUTO_TEST_SUITE(Reading)

  BOOST_AUTO_TEST_CASE(reading_convert_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/convert_error.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (false));

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

