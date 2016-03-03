# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestAddress
# include <boost/test/unit_test.hpp>

# include "test_common.hh"

# include "utils/address.hh"

using Astroid::Address;
using Astroid::AddressList;

BOOST_AUTO_TEST_SUITE(TestAddressA)

  BOOST_AUTO_TEST_CASE(quoting)
  {
    setup ();

    using Astroid::log;

    ustring ar = "A B <test@example.com>";
    Address a (ar);
    log << test << "address: " << a.full_address () << endl;

    BOOST_CHECK (ar == a.full_address ());

    ar = "\"B, A\" <test@example.com>";
    a = Address (ar);
    log << test << "address: " << a.full_address () << endl;

    BOOST_CHECK (ar == a.full_address ());

    /* string created by:
     *
     * =?utf-8?B?[insert string]?=
     *
     * echo -n "\"Mütter, A\"" | base64
     *
     * in a UTF-8 terminal.
     */
    ar = "=?utf-8?B?Ik3DvHR0ZXIsIEEi?= <a.b@c.com>";
    a = Address (ar);
    log << test << "address: " << a.full_address () << endl;

    BOOST_CHECK ("\"Mütter, A\" <a.b@c.com>" == a.full_address ());

    AddressList al (a);
    BOOST_CHECK ("\"Mütter, A\" <a.b@c.com>" == al.str());

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

