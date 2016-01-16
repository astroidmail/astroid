# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestMimeMessage
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"

using namespace std;
using Astroid::Message;
using Astroid::ustring;


BOOST_AUTO_TEST_SUITE(Reading)


  BOOST_AUTO_TEST_CASE(reading_mime_message)
  {
    setup ();

    ustring fname = "test/mail/test_mail/mime-message-no-content-type.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (true));

    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

