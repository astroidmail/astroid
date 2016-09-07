# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestDates
# include <boost/test/unit_test.hpp>
# include <gmime/gmime.h>

# include "test_common.hh"
# include "message_thread.hh"
# include "utils/date_utils.hh"

using Astroid::Message;

BOOST_AUTO_TEST_SUITE(Dates)

  BOOST_AUTO_TEST_CASE(dates_read_test)
  {
    setup ();

    {
      Message m ("test/mail/test_mail/date1.eml");

      LOG (warn) << "date1:";
      LOG (test) << "pretty_date: " << m.pretty_date ();
      LOG (test) << "pretty_verbose_date: " << m.pretty_verbose_date ();
      LOG (test) << "pretty_verbose_date (true): " << m.pretty_verbose_date (true);
    }

    {
      Message m ("test/mail/test_mail/date2.eml");

      LOG (warn) << "date2:";
      LOG (test) << "pretty_date: " << m.pretty_date ();
      LOG (test) << "pretty_verbose_date: " << m.pretty_verbose_date ();
      LOG (test) << "pretty_verbose_date (true): " << m.pretty_verbose_date (true);
    }

    teardown ();
  }


  BOOST_AUTO_TEST_CASE(dates_test_dates)
  {
    setup ();

    std::vector<ustring> dates =
    {
      "Tue, 19 Aug 2014 16:11:44 +0200",
      "Mon, 9 May 2016 13:26:57 +0000",
      "Thu, 1 Sep 2016 14:32:16 +0000"
    };


    Message a ("test/mail/test_mail/date1.eml");

    for (ustring d : dates) {
      LOG (warn) << "date: testing: " << d;

      GMimeMessage * m = a.message;
      g_object_ref (m);

      g_mime_message_set_date_as_string (m, d.c_str ());

      Message b (m);

      LOG (test) << "pretty_date: " << b.pretty_date ();
      LOG (test) << "pretty_verbose_date: " << b.pretty_verbose_date ();
      LOG (test) << "pretty_verbose_date (true): " << b.pretty_verbose_date (true);


    }


    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

