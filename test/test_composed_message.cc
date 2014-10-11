# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestCompose 
# include <boost/test/unit_test.hpp>

# include "astroid.hh"
# include "compose_message.hh"
using namespace Astroid;
using namespace std;

BOOST_AUTO_TEST_SUITE(Composing)

  void setup () {
    Astroid::astroid = new Astroid::Astroid ();
    astroid->main_test ();
  }


  BOOST_AUTO_TEST_CASE(compose_read_test)
  {
    setup ();

    ComposeMessage * c = new ComposeMessage ();

  }

BOOST_AUTO_TEST_SUITE_END()

