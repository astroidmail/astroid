# pragma once

# include "astroid.hh"
# include <string>

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
      int execute (bool undo, std::string& _stdout, std::string& _stderr); /* currently only in sync */

      ustring substitute (ustring);

    public:
      static bool pipe (ustring cmd, const ustring& _stdin, ustring& _stdout, ustring& _stderr);
  };
}

