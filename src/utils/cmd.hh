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
      Cmd (ustring prefix, ustring cmd, ustring undo_cmd);
      Cmd (ustring cmd, ustring undo_cmd);

      int run ();
      int undo ();

      bool undoable ();

    private:
      ustring prefix;
      ustring cmd;
      ustring undo_cmd;

      int execute (bool undo); /* currently only in sync */

      ustring substitute (ustring);
  };
}

