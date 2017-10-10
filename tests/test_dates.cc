# define BOOST_TEST_DYN_LINK
# define BOOST_TEST_MODULE TestDates
# include <boost/test/unit_test.hpp>
# include <gmime/gmime.h>
# include "utils/gmime/gmime-compat.h"

# include "test_common.hh"
# include "message_thread.hh"
# include "compose_message.hh"
# include "utils/date_utils.hh"

using Astroid::Message;
using Astroid::ComposeMessage;

BOOST_AUTO_TEST_SUITE(Dates)

  BOOST_AUTO_TEST_CASE(dates_read_test)
  {
    setup ();

    {
      Message m ("tests/mail/test_mail/date1.eml");

      LOG (warn) << "date1:";
      LOG (test) << "pretty_date: " << m.pretty_date ();
      LOG (test) << "pretty_verbose_date: " << m.pretty_verbose_date ();
      LOG (test) << "pretty_verbose_date (true): " << m.pretty_verbose_date (true);
    }

    {
      Message m ("tests/mail/test_mail/date2.eml");

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
    /* setlocale (LC_ALL, "C"); */

    std::vector<std::pair<ustring,ustring>> dates =
    {
      std::make_pair ("Tue, 19 Aug 2014 16:11:44 +0200", "Tue, 19 Aug 2014 16:11:44 +0200"),
      std::make_pair ("Mon, 9 May 2016 13:26:57 +0000",  "Mon, 09 May 2016 13:26:57 +0000"),
      std::make_pair ("Thu, 1 Sep 2016 14:32:16 +0000",  "Thu, 01 Sep 2016 14:32:16 +0000"),
    };


    Message a ("tests/mail/test_mail/date1.eml");

    for (auto d : dates) {
      LOG (warn) << "date: testing: " << d.first;

      GMimeMessage * m = a.message;
      g_object_ref (m);

# if (GMIME_MAJOR_VERSION < 3)
      g_mime_message_set_date_as_string (m, d.first.c_str ());
# else
      GDateTime * dt = g_mime_utils_header_decode_date (d.first.c_str ());
      g_mime_message_set_date (m, dt);
# endif

      Message b (m);

      LOG (test) << "date: " << b.date ();
      LOG (test) << "date, asctime: " << b.date_asctime ();
      LOG (test) << "pretty_date: " << b.pretty_date ();
      LOG (test) << "pretty_verbose_date: " << b.pretty_verbose_date ();
      LOG (test) << "pretty_verbose_date (true): " << b.pretty_verbose_date (true);

      BOOST_CHECK (b.date () == d.second);
    }


    LOG (test) << "Testing now date..";
    /* test compose message date setting */
    GMimeMessage * m = a.message;
    g_object_ref (m);

    /* hopefully these happen within the same second.. */
    GDateTime * now = g_date_time_new_now_local ();
    g_mime_message_set_date_now (m);

    Message b (m);
    LOG (test) << "date: " << b.date ();
    LOG (test) << "date, asctime: " << b.date_asctime ();
    LOG (test) << "pretty_date: " << b.pretty_date ();
    LOG (test) << "pretty_verbose_date: " << b.pretty_verbose_date ();
    LOG (test) << "pretty_verbose_date (true): " << b.pretty_verbose_date (true);

    const char * datefmt = "%a, %d %b %Y %T %z";
    setlocale (LC_ALL, "C");
    gchar * dt = g_date_time_format (now, datefmt);
    setlocale (LC_ALL, "");
    LOG (test) << "formatted date: " << dt;
    BOOST_CHECK (dt == b.date ());

    teardown ();
  }


BOOST_AUTO_TEST_SUITE_END()

