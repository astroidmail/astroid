# pragma once

# include "astroid.hh"
# include "log.hh"

# include <mutex>
# include <vector>
# include <glibmm/threads.h>

using namespace std;

namespace Astroid {
  class Poll {
    public:
      Poll ();

      bool poll ();

      const char * poll_script = "poll.sh";

    private:
      time_t lastpoll;
      mutex m_dopoll;

      struct LogLine {
        LogLine (LogLevel _lvl, ustring _str) :
          lvl (_lvl), str(_str) {};
        LogLevel lvl;
        ustring  str;
      };

      mutex m_log_lines;
      vector<LogLine> log_lines;

      Glib::Dispatcher signal_log;

      void do_poll ();
      void flush_log_lines ();
  };
}

