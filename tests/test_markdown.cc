# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "account_manager.hh"
# include "utils/address.hh"
# include "utils/ustring_utils.hh"

BOOST_AUTO_TEST_SUITE(Markdown)

  BOOST_AUTO_TEST_CASE(compose_read_test)
  {
    using Astroid::ComposeMessage;
    using Astroid::Message;
    setup ();

    ComposeMessage * c = new ComposeMessage ();

    ustring bdy = "# This is a test";

    c->body << bdy;
    c->markdown = true;

    c->build ();
    c->finalize ();

    BOOST_CHECK_MESSAGE (c->markdown_success, c->markdown_error);

    ustring fn = c->write_tmp ();

    Message m (fn);

    /* check plain text part */
    ustring pbdy = m.plain_text (false);
    BOOST_CHECK_MESSAGE (pbdy == bdy, "plain text matches plain text");

    /* check html part */
    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type (m.root->content_type, "multipart", "alternative"), "main message part is multipart/alternative");

    auto plain = m.root->kids[0];
    auto html  = m.root->kids[1];

    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type(plain->content_type, "text", "plain"), "first part is text/plain");
    BOOST_CHECK_MESSAGE (g_mime_content_type_is_type(html->content_type, "text", "html"), "second part is text/html");

    ustring _html = html->viewable_text (false);
    Astroid::UstringUtils::trim(_html);

    LOG (trace) << "markdown html: '" << _html << "'";

    BOOST_CHECK_MESSAGE ("<h1>This is a test</h1>" == _html, "html part is correctly constructed");

    unlink (fn.c_str ());
    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

