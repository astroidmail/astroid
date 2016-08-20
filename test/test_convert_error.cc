# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestConvertError
# include <boost/test/unit_test.hpp>
# include <iostream>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"

using std::cout;
using std::endl;
using std::locale;

using Astroid::ustring;

BOOST_AUTO_TEST_SUITE(Reading)

  BOOST_AUTO_TEST_CASE(reading_convert_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/convert_error.eml";

    Astroid::Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (false));

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(reply_convert_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/bad-convert-error.eml";

    Astroid::Message msg (fname);
    /* quote original message */
    std::ostringstream quoted;

    ustring quoting_a = ustring::compose ("Excerpts from %1's message of %2:",
        Astroid::Address(msg.sender.raw()).fail_safe_name(), msg.pretty_verbose_date());

    quoted  << quoting_a.raw ()
            << endl;

    std::string vt = msg.viewable_text(false);
    std::stringstream sstr (vt);
    while (sstr.good()) {
      std::string line;
      std::getline (sstr, line);
      quoted << ">";

      if (line[0] != '>')
        quoted << " ";

      quoted << line << endl;
    }

    ustring body = ustring(quoted.str());


    /* test writing out */
    std::string name = "test/mail/test_mail/tmp-reply-convert-error";
    LOG (test) << "writing to tmp file " << name;
    std::fstream tmpfile (name, std::fstream::out);

    tmpfile << "From: test@test.no" << endl;

    //BOOST_CHECK_THROW ( (tmpfile << body) , std::exception);
    BOOST_CHECK_NO_THROW ( tmpfile << body.raw() );

    tmpfile << endl;
    tmpfile.close ();

    LOG (test) << "removing tmp file";
    std::remove (name.c_str());

    teardown ();
  }

  /* this test should be last since it messes up the locale for the other
   * tests */
  BOOST_AUTO_TEST_CASE(glib_convert_error)
  {
    cout << "the current user preferred locale is: "
         << locale("").name() << endl;

    setlocale (LC_ALL, "");

    cout << "the current user preferred locale is: "
         << locale("").name() << endl;

    locale::global (locale (""));

    cout << "the current user preferred locale is: "
         << locale("").name() << endl;


    Glib::init ();

    BOOST_TEST_MESSAGE ("testing glib conversion of: æøå..");
    const char * input = "æøå";

    BOOST_WARN_MESSAGE (Glib::get_charset (), "The current locale is not utf8");

    std::string current_locale;
    Glib::get_charset (current_locale);
    BOOST_TEST_MESSAGE ("locale: " + current_locale);

    BOOST_CHECK_NO_THROW(ustring out = Glib::locale_to_utf8 (input));
  }

BOOST_AUTO_TEST_SUITE_END()

