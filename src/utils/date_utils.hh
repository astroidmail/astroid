# pragma once

# include "astroid.hh"

namespace Astroid {
  class Date {
    public:
      enum ClockFormat {
        TWELVE_HOURS,
        TWENTY_FOUR_HOURS,
        LOCALE_DEFAULT,
        YEAR,
      };

      // TODO: translate, might need initialization on startup,
      //       check out Geary.
      //
      // See http://developer.gnome.org/glib/2.32/glib-GDateTime.html#g-date-time-format

      static const std::vector<ustring> pretty_dates;
      static const std::vector<ustring> pretty_verbose_dates;

      // Date format for dates within the current year, i.e. Nov 8
      static ustring same_year;

      // Date format for dates within a different year, i.e. 02/04/10
      static ustring diff_year;

      static ClockFormat clock_format;

      enum CoarseDate {
        NOW,
        MINUTES,
        HOURS,
        TODAY,
        YESTERDAY,
        THIS_WEEK,
        THIS_YEAR,
        YEARS,
        FUTURE,
      };

      static bool same_day (struct tm, struct tm);
      static CoarseDate coarse_date (struct tm, struct tm, time_t );
      static CoarseDate coarse_date (time_t t);
      static ustring pretty_print (time_t );
      static ustring pretty_print_verbose (time_t, bool = false);

      static ustring asctime (time_t t);

      static void init ();
  };
}
