# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestConvertError
# include <boost/test/unit_test.hpp>

# include "test_common.hh"
# include "db.hh"
# include "message_thread.hh"
# include "glibmm.h"

using namespace std;


BOOST_AUTO_TEST_SUITE(Reading)


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

    string current_locale;
    Glib::get_charset (current_locale);
    BOOST_TEST_MESSAGE ("locale: " + current_locale);

    BOOST_CHECK_NO_THROW(ustring out = Glib::locale_to_utf8 (input));
  }

  BOOST_AUTO_TEST_CASE(reading_convert_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/convert_error.eml";

    Message m (fname);

    BOOST_CHECK_NO_THROW (m.viewable_text (false));

    teardown ();
  }

  BOOST_AUTO_TEST_CASE(reply_convert_error)
  {
    setup ();

    ustring fname = "test/mail/test_mail/bad-convert-error.eml";

    Message msg (fname);
    /* quote original message */
    ostringstream quoted;

    ustring quoting_a = ustring::compose ("Excerpts from %1's message of %2:",
        Address(msg.sender.raw()).fail_safe_name(), msg.pretty_verbose_date());

    quoted  << quoting_a.raw ()
            << endl;

    string vt = msg.viewable_text(false);
    stringstream sstr (vt);
    while (sstr.good()) {
      string line;
      getline (sstr, line);
      quoted << ">";

      if (line[0] != '>')
        quoted << " ";

      quoted << line << endl;
    }

    ustring body = ustring(quoted.str());


    /* test writing out */
    string name = tmpnam (NULL);
    Astroid::log << test << "writing to tmp file " << name << endl;
    std::fstream tmpfile (name, std::fstream::out);

    tmpfile << "From: test@test.no" << endl;

    //BOOST_CHECK_THROW ( (tmpfile << body) , std::exception);
    BOOST_CHECK_NO_THROW ( tmpfile << body.raw() );

    tmpfile << endl;
    tmpfile.close ();

    Astroid::log << test << "removing tmp file" << endl;
    remove (name);

    teardown ();
  }

BOOST_AUTO_TEST_SUITE_END()

