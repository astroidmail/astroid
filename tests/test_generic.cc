# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestGeneric
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "compose_message.hh"

# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"

BOOST_AUTO_TEST_SUITE(Generic)

  BOOST_AUTO_TEST_CASE(setup_test)
  {
    setup ();
    teardown ();
  }

  BOOST_AUTO_TEST_CASE (utils_test)
  {
    setup ();

    using namespace Astroid;

    ustring a;

    /* trim_right */
    a = " a";
    UstringUtils::trim_right (a);
    BOOST_CHECK (a == " a");

    a = " aasdfasdf ";
    UstringUtils::trim_right (a);
    BOOST_CHECK (a == " aasdfasdf");

    a = " aasdfasdf     ";
    UstringUtils::trim_right (a);
    BOOST_CHECK (a == " aasdfasdf");

    a = "    ";
    UstringUtils::trim_right (a);
    BOOST_CHECK (a == "");

    a = "n";
    UstringUtils::trim_right (a);
    BOOST_CHECK (a == "n");

    /* trim */
    a = " aasdfasdf     ";
    UstringUtils::trim (a);
    BOOST_CHECK (a == "aasdfasdf");

    a = " aasdfasdf     ";
    UstringUtils::trim (a);
    BOOST_CHECK (a == "aasdfasdf");

    a = "    aasdfasdf     ";
    UstringUtils::trim (a);
    BOOST_CHECK (a == "aasdfasdf");

    a = "        ";
    UstringUtils::trim (a);
    BOOST_CHECK (a.empty ());

    a = "";
    UstringUtils::trim (a);
    BOOST_CHECK (a.empty ());

    a = "n";
    UstringUtils::trim_left (a);
    BOOST_CHECK (a == "n");

    a = "n";
    UstringUtils::trim (a);
    BOOST_CHECK (a == "n");


    /* vector */
    a = "asd, bgd ,";
    auto v = VectorUtils::split_and_trim (a, ",");
    BOOST_CHECK (v[0] == "asd");
    BOOST_CHECK (v[1] == "bgd");
    BOOST_CHECK (v.size () == 2);

    a = "asd, ,bgd ,";
    v = VectorUtils::split_and_trim (a, ",");
    BOOST_CHECK (v[0] == "asd");
    BOOST_CHECK (v[1] == "bgd");
    BOOST_CHECK (v.size () == 2);

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

