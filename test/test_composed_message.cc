# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"
# include "message_thread.hh"

BOOST_AUTO_TEST_SUITE(Composing)

  BOOST_AUTO_TEST_CASE(compose_read_test)
  {
    using Astroid::ComposeMessage;
    setup ();

    ComposeMessage * c = new ComposeMessage ();

    delete c;

    teardown ();
  }

  BOOST_AUTO_TEST_CASE (compose_test_body)
  {
    using Astroid::ComposeMessage;
    using Astroid::Message;
    setup ();

    ComposeMessage * c = new ComposeMessage ();

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    LOG (trace) << "cm: writing utf-8 text to message body: " << bdy;
    c->body << bdy;

    c->build ();
    c->finalize ();
    ustring fn = c->write_tmp ();

    delete c;

    Message m (fn);

    ustring rbdy = m.viewable_text (false);

    BOOST_CHECK_MESSAGE (bdy == rbdy, "message reading produces the same output as compose message input");

    unlink (fn.c_str ());

    teardown ();

  }

  BOOST_AUTO_TEST_CASE (compose_test_references)
  {
    using Astroid::ComposeMessage;
    using Astroid::Message;
    setup ();

    ComposeMessage * c = new ComposeMessage ();
    ustring ref = "test-ref";

    c->set_references (ref);

    BOOST_CHECK_MESSAGE (ref == g_mime_object_get_header (GMIME_OBJECT(c->message), "References"), "message references is set");

    c->set_references ("");

    BOOST_CHECK_MESSAGE (FALSE == g_mime_header_list_contains (g_mime_object_get_header_list (GMIME_OBJECT(c->message)), "References"), "message references is set when empty");

    /* try to set empty reference when references are empty already */
    c->set_references ("");

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

