# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestMimeMessage
# include <boost/test/unit_test.hpp>
# include <boost/filesystem.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "compose_message.hh"
# include "account_manager.hh"
# include "glibmm.h"

using namespace std;
using Astroid::Message;
using Astroid::ustring;

namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(Reading)


  BOOST_AUTO_TEST_CASE(reading_mime_message)
  {
    setup ();

    ustring fname = "test/mail/test_mail/mime-message-no-content-type.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (true));

    teardown ();
  }

  BOOST_AUTO_TEST_CASE (write_mm_attachment_signature)
  {
    /* #237 and #239 */

    using Astroid::ComposeMessage;
    using Astroid::Account;

    setup ();

    Account a = astroid->accounts->accounts[0];
    a.signature_file = bfs::path ("test/test_home/signature.txt");
    a.has_signature  = true;
    a.signature_attach = false;

    ComposeMessage * c = new ComposeMessage ();

    std::shared_ptr<ComposeMessage::Attachment> mm (new ComposeMessage::Attachment ("test/mail/test_mail/multipart.eml"));
    c->add_attachment (mm);

    c->set_from (&a);
    c->include_signature = true;
    c->build ();
    c->finalize ();

    c->write ("test/mail/test_mail/out_mm_sig.eml");

    ustring fn = c->write_tmp ();

    delete c;

    unlink (fn.c_str ());
    unlink ("test/mail/test_mail/out_mm_sig.eml");

    teardown ();

  }


BOOST_AUTO_TEST_SUITE_END()

