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

BOOST_AUTO_TEST_SUITE_END()

