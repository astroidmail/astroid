# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCrypto
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"
# include "crypto.hh"
# include "message_thread.hh"
# include "account_manager.hh"
# include "db.hh"


BOOST_AUTO_TEST_SUITE(GPGEncryption)

  BOOST_AUTO_TEST_CASE(crypto_compose_simple_test)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    setup ();

    Account a = astroid->accounts->accounts[0];
    LOG (trace) << "cy: account gpg: " << a.gpgkey;

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
    setup ();

    Account a = astroid->accounts->accounts[0];
    LOG (trace) << "cy: account gpg: " << a.gpgkey;
    a.email = "gaute@astroidmail.bar";

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("astrid@astroidmail.bar");
    c->encrypt =  true;
    c->sign = false;

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    LOG (trace) << "cm: writing utf-8 text to message body: " << bdy;
    c->body << bdy;

    LOG (trace) << "build:";
    c->build ();
    LOG (trace) << "finalize:";
    c->finalize ();
    ustring fn = c->write_tmp ();

    BOOST_CHECK_MESSAGE (c->encryption_success == true, "encryption should be successful");
    LOG (test) << "cm: encryption error: " << c->encryption_error;

    LOG (test) << "cm: encrypted content: ";
    LOG (test) << g_mime_object_to_string (GMIME_OBJECT(c->message));

    LOG (test) << "cm: deleting ComposeMessage..";

    delete c;

    Message m (fn);
    ustring rbdy = m.viewable_text (false);

    BOOST_CHECK_MESSAGE (bdy == rbdy, "message reading produces the same output as compose message input");

    unlink (fn.c_str ());

    teardown ();
  }

  BOOST_AUTO_TEST_CASE (crypto_compose_test_sign_body)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::Message;
    setup ();

    Account a = astroid->accounts->accounts[0];
    LOG (test) << "cy: account gpg: " << a.gpgkey;
    a.email = "gaute@astroidmail.bar";

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("who@astroidmail.bar"); // no pub key
    c->encrypt = false;
    c->sign = true;

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    LOG (test) << "cm: writing utf-8 text to message body: " << bdy;
    c->body << bdy;

    c->build ();
    c->finalize ();
    ustring fn = c->write_tmp ();

    BOOST_CHECK_MESSAGE (c->encryption_success == true, "encryption should be successful");
    LOG (test) << "cm: encryption error: " << c->encryption_error;

    LOG (test) << "cm: encrypted content: ";
    LOG (test) << g_mime_object_to_string (GMIME_OBJECT(c->message));

    LOG (test) << "cm: deleting ComposeMessage..";

    delete c;

    Message m (fn);

    ustring rbdy = m.viewable_text (false);

    BOOST_CHECK_MESSAGE (bdy == rbdy, "message reading produces the same output as compose message input");

    unlink (fn.c_str ());

    teardown ();
  }


  BOOST_AUTO_TEST_CASE (crypto_missing_pub)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::Message;
    setup ();

    Account a = astroid->accounts->accounts[0];
    LOG (test) << "cy: account gpg: " << a.gpgkey;
    a.email = "gaute@astroidmail.bar";

    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("err@astroidmail.bar"); // unknown
    c->encrypt = true;

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    c->body << bdy;

    LOG (test) << "build:";
    c->build ();

    LOG (test) << "finalize:";
    c->finalize ();

    BOOST_CHECK_MESSAGE (c->encryption_success == false, "encryption should fail");
    LOG (test) << "cm: encryption error: " << c->encryption_error;

    LOG (test) << "cm: encrypted content: ";
    LOG (test) << g_mime_object_to_string (GMIME_OBJECT(c->message));

    delete c;


    teardown ();
  }

  BOOST_AUTO_TEST_CASE (crypto_too_many_open_files)
  {
    using Astroid::ComposeMessage;
    using Astroid::Account;
    using Astroid::Message;
    using Astroid::MessageThread;
    using Astroid::message_error;
    using Astroid::Db;
    using Astroid::NotmuchThread;
    setup ();

    Account a = astroid->accounts->accounts[0];
    LOG (trace) << "cy: account gpg: " << a.gpgkey;
    a.email = "gaute@astroidmail.bar";

    /* create a test message */
    ComposeMessage * c = new ComposeMessage ();
    c->set_from (&a);
    c->set_to ("astrid@astroidmail.bar");
    c->encrypt =  true;
    c->sign = true;

    ustring bdy = "This is test: æøå.\n > testing\ntesting\n...";

    c->body << bdy;
    c->set_subject ("test gpg e-mail");
    ustring mid = "test-gpg-1@astroid";
    c->set_id (mid);

    c->build ();
    c->finalize ();

    ustring fn = "test/mail/test_mail/gpg-test-1.eml";
    c->write (fn);
    system ("notmuch new; notmuch search \"*\"");

    BOOST_CHECK_MESSAGE (c->encryption_success == true, "encryption should be successful");
    LOG (test) << "cm: deleting ComposeMessage..";

    delete c;

    Message *m;

    {
      Db db(Db::DATABASE_READ_ONLY);
      db.on_message (mid, [&](notmuch_message_t * msg) {
          LOG (test) << "trying to open encrypted file.";

          m = new Message (msg, 0);

          });
    }

    /* check that body matches */
    ustring rbdy = m->viewable_text (false);
    BOOST_CHECK_MESSAGE (bdy == rbdy, "message reading produces the same output as compose message input");

    /* notmuch thread id */
    ustring tid = m->tid;
    delete m;

    refptr<NotmuchThread> nt;

    LOG (test) << "trying to open thread: " << tid;

    {
      Db db(Db::DATABASE_READ_ONLY);
      db.on_thread (tid, [&](notmuch_thread_t * nmt) {

          BOOST_CHECK (nmt != NULL);
          nt = refptr<NotmuchThread> (new NotmuchThread (nmt));

          });
    }


    /* Okay, let's see if we can provoke 'Too many open files' in GPG */
    int tries = 0;
    bool failed = false;

    try {
      for (int i = 0; i < 100; i ++) {
        LOG (info) << "loading message thread..";
        refptr<MessageThread> mthread = refptr<MessageThread>(new MessageThread (nt));
        Db db (Db::DATABASE_READ_ONLY);
        mthread->load_messages (&db);
        tries++;
      }
    } catch (Astroid::message_error &ex) {
      LOG (error) << "caught too many files error: " << ex.what ();
      failed = true;
    }

    /* let's try without using refptr */
    /*
    LOG (info) << "loading without using refptr..";
    try {
      for (int i = 0; i < 1200; i ++) {
        LOG (info) << "loading message thread..";
        MessageThread mthread (nt);
        Db db (Db::DATABASE_READ_ONLY);
        mthread.load_messages (&db);
        tries++;
      }
    } catch (Astroid::message_error &ex) {
      LOG (error) << "caught too many files error: " << ex.what ();
      failed = true;
    }
    */

    LOG (warn) << "opened a GPG e-mail: " << tries << " times.";

    BOOST_CHECK_MESSAGE (failed == false, "Caught 'Too many open files error'");

    /* removing test encrypted file */
    unlink (fn.c_str ());
    system ("notmuch new");

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

