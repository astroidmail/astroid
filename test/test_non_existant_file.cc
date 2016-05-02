# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestOpenError
# include <boost/test/unit_test.hpp>
# include <boost/filesystem.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"
# include "chunk.hh"

using namespace boost::filesystem;

BOOST_AUTO_TEST_SUITE(Reading)

  BOOST_AUTO_TEST_CASE(open_error)
  {
    setup ();

    std::string fname = "test/mail/test_mail/this-one-should-not-exist.eml";

    BOOST_CHECK (!exists(fname));

    BOOST_CHECK_THROW(Astroid::Message m (fname), Astroid::message_error);

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(out_of_sync)
  {
    using Astroid::log;
    using Astroid::test;
    using std::endl;
    using Astroid::Message;
    using Astroid::Db;
    using Astroid::AddressList;
    using Astroid::ustring;

    setup ();

    /* open other email and make copy */
    Message om ("test/mail/test_out_of_sync.eml");
    om.save_to ("test/mail/test_mail/oos.eml");

    /* update notmuch */
    system ("notmuch new");

    /* test if file can be read now */
    Message * mm;
    BOOST_CHECK_NO_THROW (mm = new Message ("test/mail/test_mail/oos.eml"));
    Astroid::log << test << "other: " << AddressList (mm->other_to ()).str () << endl;
    delete mm;

    /* remove it without updating notmuch */
    unlink ("test/mail/test_mail/oos.eml");

    /* try to open file using notmuch */
    Message * oos;

    ustring mid = "oos@asdf.com";
    Db db(Db::DATABASE_READ_ONLY);
    db.on_message (mid, [&](notmuch_message_t * msg) {
        Astroid::log << test << "trying to open deleted file." << endl;

        oos = new Message (msg, 0);

        Astroid::log << test << "deleted file opened." << endl;

        });

    /* testing various methods */
    Astroid::log << test << "message: testing methods on out-of-sync message." << endl;

    oos->save_to ("test/mail/test_mail/wont-work.eml");

    Astroid::log << test << "sender: " << oos->sender << endl;
    Astroid::log << test << "text: " << oos->viewable_text (false) << endl;

    /* these do not seem to be cached */
    Astroid::log << test << "to: " << AddressList (oos->to()).str() << endl;
    Astroid::log << test << "cc: " << AddressList (oos->cc()).str() << endl;
    Astroid::log << test << "bcc: " << AddressList (oos->bcc()).str() << endl;
    Astroid::log << test << "other: " << AddressList (oos->other_to ()).str () << endl;
    Astroid::log << test << "date: " << oos->date () << endl;

    Astroid::log << test << "pretty date: " << oos->pretty_verbose_date() << endl;

    oos->contents ();
    oos->attachments ();
    oos->mime_messages ();

    /* update notmuch */
    system ("notmuch new");

    teardown ();
  }
BOOST_AUTO_TEST_SUITE_END()

