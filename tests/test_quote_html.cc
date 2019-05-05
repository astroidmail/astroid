# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "message_thread.hh"
# include "utils/ustring_utils.hh"
# include "config.hh"

BOOST_AUTO_TEST_SUITE(QuoteHtml)

  BOOST_AUTO_TEST_CASE(quote_html)
  {
    using Astroid::Message;
    setup ();

    ustring fname = "tests/mail/test_mail/only-html.eml";

    Message m (fname);
    ustring quoted = m.quote ();

    LOG (trace) << "quoted plain text: " << quoted;

    Astroid::UstringUtils::trim (quoted);

    ustring target = R"(1. save an email as file.eml
 2. write a new email
 3. attach file.eml
 4. save as draft
 5. quit astroid
 6. open draft again
 7. edit the draft
 8. here it is.

—
You are receiving this because you commented.
Reply to this email directly, view it on GitHub, or mute the thread.*)";

    BOOST_CHECK (quoted == target);

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(quote_html_convert_error)
  {
    using Astroid::Message;
    setup ();

    // TODO: Does not work with lynx
    /* const_cast<ptree&>(astroid->config()).put ("mail.reply.quote_processor", "lynx -dump -stdin"); */

    ustring fname = "tests/mail/test_mail/isspace-fail-utf-8.eml";

    Message m (fname);
    ustring quoted = m.quote ();

    LOG (trace) << "quoted plain text: " << quoted;

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

