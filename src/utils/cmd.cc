# include "cmd.hh"
# include "astroid.hh"
# include "log.hh"
# include "vector_utils.hh"

# include <glibmm.h>

using std::endl;
using std::string;

namespace Astroid {
  Cmd::Cmd (ustring _prefix, ustring _cmd) {
    prefix = _prefix + ": ";
    cmd = _cmd;
  }

  Cmd::Cmd (ustring _cmd) {
    cmd = _cmd;
  }

  int Cmd::run () {
    log << info << "cmd: running: " << cmd << endl;
    string _stdout;
    string _stderr;
    int exit;

    string _cmd = cmd;
    Glib::spawn_command_line_sync (_cmd, &_stdout, &_stderr, &exit);

    if (!_stdout.empty ()) {
      for (auto &l : VectorUtils::split_and_trim (_stdout, "\n")) {
        if (!l.empty ()) log << debug << "cmd: " << prefix << l << endl;
      }
    }

    if (!_stderr.empty ()) {
      for (auto &l : VectorUtils::split_and_trim (_stderr, "\n")) {
        if (!l.empty ()) log << error << "cmd: " << prefix << l << endl;
      }
    }

    return exit;
  }
}


