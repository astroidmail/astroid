# pragma once

# include "astroid.hh"
# include "log.hh"

# include <mutex>
# include <glibmm/threads.h>
# include <glibmm/iochannel.h>

using namespace std;

namespace Astroid {
  class Poll {
    public:
      Poll (bool auto_polling_enabled);

      bool poll ();

      const char * poll_script = "poll.sh";

    private:
      mutex m_dopoll;

      int poll_interval = 0;

      void do_poll ();
      bool periodic_polling ();

      chrono::time_point<chrono::steady_clock> t0; // start time of poll

      int pid;
      int stdin;
      int stdout;
      int stderr;
      refptr<Glib::IOChannel> ch_stdout;
      refptr<Glib::IOChannel> ch_stderr;

      bool log_out (Glib::IOCondition);
      bool log_err (Glib::IOCondition);
      void child_watch (GPid, int);
  };
}

