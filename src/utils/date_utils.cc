# include <iostream>

# include <boost/property_tree/ptree.hpp>
# include <glibmm/datetime.h>
# include <time.h>

# include "astroid.hh"
# include "config.hh"
# include "date_utils.hh"

using namespace std;
using boost::property_tree::ptree;

namespace Astroid {

  ustring Date::asctime (time_t t) {
    struct tm * temp_t = localtime (&t);
    struct tm local_time = *temp_t;

    char * asc = std::asctime (&local_time);

    return ustring (asc);
  }

  /*
   * create a pretty string in local time.
   *
   * mostly ported from Gearys pretty printing:
   * https://git.gnome.org/browse/geary/tree/src/client/util/util-date.vala
   *
   */

  const vector<ustring> Date::pretty_dates =
    {
      "%l:%M %P", // Datetime format for 12-hour time, i.e. 8:31 am
      "%H:%M",    // Datetime format for 24-hour time, i.e. 16:35
      "%l:%M %P", // Datetime format for the locale default, i.e. 8:31 am or 16:35,
      // year format will be covered specially.
    };
  const vector<ustring> Date::pretty_verbose_dates =
    {
      // Verbose datetime format for 12-hour time, i.e. November 8, 2010 8:42 am
      "%B %-e, %Y %-l:%M %P",

      // Verbose datetime format for 24-hour time, i.e. November 8, 2010 16:35
      "%B %-e, %Y %-H:%M",

      // Verbose datetime format for the locale default (full month, day and time)
      "%B %-e, %Y %-l:%M %P",

      // Verbose datetime format for year, 24-hour time, i.e. November 8, 2010 16:35
      "%B %-e, %Y %-H:%M",
    };

  // Date format for dates within the current year, i.e. Nov 8
  ustring Date::same_year;

  // Date format for dates within a different year, i.e. 02/04/10
  ustring Date::diff_year;

  Date::ClockFormat Date::clock_format;

  ustring Date::pretty_print (time_t t) {
    struct tm * temp_t = localtime (&t);
    struct tm local_time = *temp_t;

    time_t now  = time (NULL);
    time_t diff = now - t;

    CoarseDate cd = coarse_date (t);

    ustring fmt;
	if (clock_format == ClockFormat::YEAR) {
		switch (cd) {
		case CoarseDate::YEARS:
		case CoarseDate::FUTURE:
			fmt = diff_year;
			break;
		default:
			fmt = same_year;
			break;
		}
	}
	else {
		switch (cd) {
		case CoarseDate::NOW:
			return "Now";

		case CoarseDate::MINUTES:
			return ustring::compose("%1m ago", (unsigned long) (diff / 60));

		case CoarseDate::HOURS:
			return ustring::compose("%1h ago", (unsigned long) (diff / (60 * 60)));

		case CoarseDate::TODAY:
			fmt = pretty_dates[clock_format];
			break;

		case CoarseDate::YESTERDAY:
			return "Yesterday";

		case CoarseDate::THIS_WEEK:
			// Date format that shows the weekday (Monday, Tuesday, ...)
			// See http://developer.gnome.org/glib/2.32/glib-GDateTime.html#g-date-time-format
			fmt = "%A";
			break;

		case CoarseDate::THIS_YEAR:
			fmt = same_year;
			break;

		case CoarseDate::YEARS:
		case CoarseDate::FUTURE:
		default:
			fmt = diff_year;
			break;
		}
	}

    Glib::DateTime dt = Glib::DateTime::create_local (
        local_time.tm_year + 1900,
        local_time.tm_mon + 1,
        local_time.tm_mday,
        local_time.tm_hour,
        local_time.tm_min,
        local_time.tm_sec);

    return dt.format (fmt);
  }

  ustring Date::pretty_print_verbose (time_t t, bool include_short) {

    struct tm * temp_t = localtime (&t);
    struct tm local_time = *temp_t;

    Glib::DateTime dt = Glib::DateTime::create_local (
        local_time.tm_year + 1900,
        local_time.tm_mon + 1,
        local_time.tm_mday,
        local_time.tm_hour,
        local_time.tm_min,
        local_time.tm_sec);

    ustring v = dt.format (pretty_verbose_dates[clock_format]);

    CoarseDate cd = coarse_date (t);
    if (include_short && (cd < CoarseDate::THIS_YEAR)) {
      v = v + " (" + pretty_print (t) + ")";
    }

    return v;
  }

  void Date::init () {
    LOG (info) << "date: init.";

    ptree config = astroid->config ("general.time");
    string c_f = config.get<string>("clock_format");

    if (c_f == "24h") {
      clock_format = ClockFormat::TWENTY_FOUR_HOURS;
    } else if (c_f == "12h") {
      clock_format = ClockFormat::TWELVE_HOURS;
    } else if (c_f == "year") {
      clock_format = ClockFormat::YEAR;
    } else if (c_f == "local") {
      clock_format = ClockFormat::LOCALE_DEFAULT;
    } else {
      LOG (error) << "date: error: unrecognized clock format in config: " << c_f << ", should be either 'local', '24h', '12h' or 'year'.";
      clock_format = ClockFormat::LOCALE_DEFAULT;
    }

    /* same year format */
    same_year = config.get<string>("same_year");

    /* diff year */
    diff_year = config.get<string>("diff_year");
  }

  Date::CoarseDate Date::coarse_date (time_t t) {
    struct tm * temp_t = localtime (&t);
    struct tm local_time = *temp_t;

    time_t now  = time (NULL);
    time_t diff = now - t;

    struct tm now_time;
    temp_t = localtime (&now);
    now_time = *temp_t;

    return coarse_date (local_time, now_time, diff);
  }

  Date::CoarseDate Date::coarse_date (struct tm t, struct tm now, time_t diff) {

    if (same_day (t, now)) {
      if (diff < 0) {
        return CoarseDate::FUTURE;
      }

      if (diff < 60) {
        return CoarseDate::NOW;
      }

      if (diff < (60 * 60)) {
        return CoarseDate::MINUTES;
      }

      if (diff < (12 * 60 * 60)) {
        return CoarseDate::HOURS;
      }


      return CoarseDate::TODAY;

    } else {
      if (diff < 0) {
        return CoarseDate::FUTURE;
      }

      /* make copy of t */
      time_t t_t = mktime (&t);

      // add one day
      t_t += 24 * 60 * 60;
      struct tm * ttemp_t = localtime (&t_t);
      struct tm ttemp = *ttemp_t;

      if (same_day (ttemp, now)) {
        return CoarseDate::YESTERDAY;
      }

      // add another five days
      t_t += 5 * 24 * 60 * 60;
      ttemp_t = localtime (&t_t);
      ttemp = *ttemp_t;
      time_t ndiff = mktime(&ttemp) - mktime(&now);
      if (same_day (ttemp, now) || ndiff > 0) {
        return CoarseDate::THIS_WEEK;
      }

      if (t.tm_year == now.tm_year) {
        return CoarseDate::THIS_YEAR;
      } else {
        return CoarseDate::YEARS;
      }
    }
  }

  bool Date::same_day (struct tm t1, struct tm t2) {
    return (t1.tm_year == t2.tm_year) &&
           (t1.tm_mon == t2.tm_mon) &&
           (t1.tm_mday  == t2.tm_mday);
  }
}

