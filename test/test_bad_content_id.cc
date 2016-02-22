# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestReading
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "glibmm.h"
# include "log.hh"

using namespace std;
using Astroid::Message;
using Astroid::MessageThread;
using Astroid::Chunk;
using Astroid::ustring;
using Astroid::test;

BOOST_AUTO_TEST_SUITE(Reading)


  BOOST_AUTO_TEST_CASE(BadContentId)
  {
    setup ();

    ustring fname = "test/mail/test_mail/bad-content-part-id.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (true));

    /* the first part is probablematic */
    /* refptr<Chunk> c = m.root->kids[0]; */
    for (auto &c : m.mime_messages ()) {
      /* Astroid::log << test << "chunk: " << c->get_content_type () << endl; */

      refptr<MessageThread> mt = refptr<MessageThread> (new MessageThread ());
      mt->add_message (c);
    }

    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

