# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestReading
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "glibmm.h"

using std::endl;
using Astroid::Message;
using Astroid::MessageThread;
using Astroid::Chunk;
using Astroid::ustring;

BOOST_AUTO_TEST_SUITE(Reading)

  /*
   * the Content-Type: .. at line 30: seems to not be captured by gmime,
   * removing the newline (as in BadContentId2) fixes the issue.
   *
   */

  BOOST_AUTO_TEST_CASE(BadContentId2)
  {
    setup ();

    ustring fname = "test/mail/test_mail/bad-content-part-id-2.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (true));

    /* the first part is probablematic */
    /* refptr<Chunk> c = m.root->kids[0]; */
    for (auto &c : m.mime_messages ()) {
      LOG (test) << "chunk: " << c->id
        << ", viewable: " << c->viewable
        << ", mime_message: " << c->mime_message
       ;

      /*
      std::string content ((char *) c->contents ()->get_data ());
      LOG (test) <<  content;
      */

      refptr<MessageThread> mt = refptr<MessageThread> (new MessageThread ());
      mt->add_message (c);
    }

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(BadContentId1)
  {
    setup ();

    ustring fname = "test/mail/test_mail/bad-content-part-id.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (true));

    /* the first part is probablematic */
    /* refptr<Chunk> c = m.root->kids[0]; */
    for (auto &c : m.mime_messages ()) {
      LOG (test) << "chunk: " << c->id
        << ", viewable: " << c->viewable
        << ", mime_message: " << c->mime_message
       ;

      /*
      std::string content ((char *) c->contents ()->get_data ());
      LOG (test) <<  content;
      */

      refptr<MessageThread> mt = refptr<MessageThread> (new MessageThread ());
      mt->add_message (c);
    }

    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

