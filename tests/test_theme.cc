# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestTheme
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "glibmm.h"

# include "modes/thread_view/theme.hh"

using namespace std;
using Astroid::ustring;


BOOST_AUTO_TEST_SUITE(Theme)

  BOOST_AUTO_TEST_CASE(loading_theme)
  {
    setup ();

    Astroid::Theme * t;
    BOOST_CHECK_NO_THROW (t = new Astroid::Theme ());

    ustring css = t->thread_view_css;
    ustring html = t->thread_view_html;

    delete t;

    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

