# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestGeneric
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"
using namespace std;

BOOST_AUTO_TEST_SUITE(Generic)

  BOOST_AUTO_TEST_CASE(setup_test)
  {
    setup ();
    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

