# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestNoNewline
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"

using namespace std;


BOOST_AUTO_TEST_SUITE(Reading)

  BOOST_AUTO_TEST_CASE(reading_no_new_line_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/no-nl.eml";

    Message m (fname);

    ustring text =  m.viewable_text(false);
    BOOST_CHECK (text.find ("line-ignored") != ustring::npos);

    ustring html = m.viewable_text(true);
    BOOST_CHECK (html.find ("line-ignored") != ustring::npos);

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(reading_no_new_line_after_link)
  {
    /* this fails if there is a link at the end of a part (plain-text at
     * least) that does not have any other text before it */

    setup ();

    ustring fname = "test/mail/test_mail/no-nl-link.eml";

    Message m (fname);

    ustring text =  m.viewable_text(false);
    BOOST_CHECK (text.find ("line-ignored.com") != ustring::npos);

    ustring html = m.viewable_text(true);
    cout << "html: " << html << endl;
    BOOST_CHECK (html.find ("line-ignored.com") != ustring::npos);


    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

