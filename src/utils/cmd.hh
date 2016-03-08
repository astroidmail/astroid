# pragma once

# include "astroid.hh"

# include <mutex>
# include <chrono>
# include <glibmm/threads.h>
# include <glibmm/iochannel.h>

namespace Astroid {
  class Cmd {
    public:
      Cmd ();
      Cmd (ustring prefix, ustring cmd);
      Cmd (ustring cmd);

      int run (); /* currently only in sync */

    private:
      ustring prefix;
      ustring cmd;

      ustring substitute (ustring);
  };
}

