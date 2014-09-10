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
      mutex m_dopoll;

      void do_poll ();
      void flush_log_lines ();
  };
}

