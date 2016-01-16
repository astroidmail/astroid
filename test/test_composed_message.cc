# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose 
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"

BOOST_AUTO_TEST_SUITE(Composing)

  BOOST_AUTO_TEST_CASE(compose_read_test)
  {
    using Astroid::ComposeMessage;
    setup ();

    ComposeMessage * c = new ComposeMessage ();

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

