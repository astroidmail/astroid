# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCrypto
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"
# include "crypto.hh"
# include "message_thread.hh"
# include "account_manager.hh"


BOOST_AUTO_TEST_SUITE(PGPEncryption)

  BOOST_AUTO_TEST_CASE(crypto_compose_simple_test)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::log;
    setup ();

    Account a = astroid->accounts->accounts[0];
    log << test << "cy: account gpg: " << a.gpgkey << endl;

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);

    delete c;

    teardown ();
  }

  BOOST_AUTO_TEST_CASE (crypto_compose_test_encrypt_body)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::Message;
    using Astroid::log;
    setup ();

    Account a = astroid->accounts->accounts[0];
    log << test << "cy: account gpg: " << a.gpgkey << endl;
    a.email = "gaute@astroidmail.bar";

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("astrid@astroidmail.bar");
    c->encrypt =  true;
    c->sign = false; // passphrase doesn't work with gpg2

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    log << test << "cm: writing utf-8 text to message body: " << bdy << endl;
    c->body << bdy;

    c->build ();
    c->finalize ();
    ustring fn = c->write_tmp ();

    BOOST_CHECK_MESSAGE (c->encryption_success == true, "encryption should be successful");
    log << test << "cm: encryption error: " << c->encryption_error << endl;

    log << test << "cm: encrypted content: " << endl;
    log << test << g_mime_object_to_string (GMIME_OBJECT(c->message)) << endl;

    log << test << "cm: deleting ComposeMessage.." << endl;

    delete c;

    /* need passphrase, doesn't work yet */

    /*
    Message m (fn);

    ustring rbdy = m.viewable_text (false);

    BOOST_CHECK_MESSAGE (bdy == rbdy, "message reading produces the same output as compose message input");
    */

    unlink (fn.c_str ());

    teardown ();
  }

  /*
  BOOST_AUTO_TEST_CASE (crypto_compose_test_sign_body)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::Message;
    using Astroid::log;
    setup ();

    Account a = astroid->accounts->accounts[0];
    log << test << "cy: account gpg: " << a.gpgkey << endl;
    a.email = "gaute@astroidmail.bar";

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("who@astroidmail.bar"); // no pub key
    c->encrypt = false;
    c->sign = false;

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    log << test << "cm: writing utf-8 text to message body: " << bdy << endl;
    c->body << bdy;

    c->build ();
    c->finalize ();
    ustring fn = c->write_tmp ();

    BOOST_CHECK_MESSAGE (c->encryption_success == true, "encryption should be successful");
    log << test << "cm: encryption error: " << c->encryption_error << endl;

    log << test << "cm: encrypted content: " << endl;
    log << test << g_mime_object_to_string (GMIME_OBJECT(c->message)) << endl;

    log << test << "cm: deleting ComposeMessage.." << endl;

    delete c;

    Message m (fn);

    ustring rbdy = m.viewable_text (false);

    BOOST_CHECK_MESSAGE (bdy == rbdy, "message reading produces the same output as compose message input");

    unlink (fn.c_str ());

    teardown ();
  }
  */


  BOOST_AUTO_TEST_CASE (crypto_missing_pub)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::Message;
    using Astroid::log;
    setup ();

    Account a = astroid->accounts->accounts[0];
    log << test << "cy: account gpg: " << a.gpgkey << endl;
    a.email = "gaute@astroidmail.bar";

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("err@astroidmail.bar"); // unknown
    c->encrypt = true;

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    c->body << bdy;

    c->build ();
    c->finalize ();

    BOOST_CHECK_MESSAGE (c->encryption_success == false, "encryption should fail");
    log << test << "cm: encryption error: " << c->encryption_error << endl;

    log << test << "cm: encrypted content: " << endl;
    log << test << g_mime_object_to_string (GMIME_OBJECT(c->message)) << endl;

    delete c;


    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

