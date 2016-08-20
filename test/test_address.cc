# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestAddress
# include <boost/test/unit_test.hpp>

# include "test_common.hh"

# include "utils/address.hh"
# include "message_thread.hh"

using Astroid::Address;
using Astroid::AddressList;
using Astroid::Message;

BOOST_AUTO_TEST_SUITE(TestAddressA)

  BOOST_AUTO_TEST_CASE(quoting)
  {
    setup ();

    ustring ar = "A B <test@example.com>";
    Address a (ar);
    LOG (test) << "address: " << a.full_address ();

    BOOST_CHECK (ar == a.full_address ());

    ar = "\"B, A\" <test@example.com>";
    a = Address (ar);
    LOG (test) << "address: " << a.full_address ();

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
    LOG (test) << "address: " << a.full_address ();

    BOOST_CHECK ("\"Mütter, A\" <a.b@c.com>" == a.full_address ());

    AddressList al (a);
    BOOST_CHECK ("\"Mütter, A\" <a.b@c.com>" == al.str());

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(utf8_isspace)
  {
    setup ();

    Message m ("test/mail/test_mail/isspace-fail-utf-8.eml");
    LOG (test) << m.sender;

    Address ma (m.sender);
    ustring b = ma.fail_safe_name ();
    LOG (test) << "address: " << b;

    Address testspace (" Hey", "");
    BOOST_CHECK ("Hey" == testspace.fail_safe_name ());

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

