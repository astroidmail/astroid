# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"
# include "message_thread.hh"
# include "account_manager.hh"
# include "utils/address.hh"
# include "utils/ustring_utils.hh"

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

  BOOST_AUTO_TEST_CASE (send_messages)
  {

    using Astroid::ComposeMessage;
    using Astroid::Message;
    using Astroid::Account;
    using Astroid::AddressList;
    using Astroid::UstringUtils;

    setup ();

    Account a = astroid->accounts->accounts[0];
    a.email   = "gaute@astroidmail.bar";
    a.signature_file = "test/test_home/signature.txt";

    /* read signature */
    std::ifstream s (a.signature_file.c_str ());
    std::ostringstream sf;
    sf << s.rdbuf ();
    s.close ();
    ustring signature = sf.str ();

    LOG (trace) << "cm: account gpg: " << a.gpgkey;
    LOG (debug) << "cm: signature file: " << a.signature_file;

    ustring fname = "test/mail/test_mail/test-output.eml";
    a.sendmail = "tee " + fname;
    LOG (debug) << "cm: sendmail: " << a.sendmail;

    for (int i = 0; i < 100; i++) {
      ComposeMessage * c = new ComposeMessage ();

      ustring to      = "bar@astroidmail.bar";
      ustring subject = "æøå adfasdf asdf";
      ustring id      = "1@test";

      c->set_from (&a);
      c->set_id (id);
      c->set_to (to);
      c->set_subject (subject);

      /* set body */
      ustring body = UstringUtils::random_alphanumeric (1000) + "\n   >>" + UstringUtils::random_alphanumeric (1500);
      c->body << body;

      c->include_signature = true;

      c->build ();
      c->finalize ();

      if (i % 2 == 0) {
        c->send_threaded ();
      } else {
        c->send ();
      }

      delete c;

      /* read message */
      Message m (fname);

      BOOST_CHECK (m.subject == subject);
      BOOST_CHECK (AddressList(m.to()).str () == to);
      BOOST_CHECK (m.mid == id);
      BOOST_CHECK (m.viewable_text (false) == (body + signature));

      unlink (fname.c_str ());
    }

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

