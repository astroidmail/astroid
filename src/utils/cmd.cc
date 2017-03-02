# include "cmd.hh"
# include "astroid.hh"
# include "vector_utils.hh"
# include "config.hh"

# include <glibmm.h>
# include <boost/filesystem.hpp>

using std::endl;
using std::string;
namespace bfs = boost::filesystem;

namespace Astroid {
  Cmd::Cmd (ustring _prefix, ustring _cmd) : Cmd (_cmd) {
    prefix = _prefix + ": ";
  }

  Cmd::Cmd (ustring _cmd) {
    cmd = substitute (_cmd);
  }

  Cmd::Cmd () {

  }

  int Cmd::run () {
    LOG (info) << "cmd: running: " << cmd;
    string _stdout;
    string _stderr;
    int exit;

    string _cmd = cmd;
    try {
      Glib::spawn_command_line_sync (_cmd, &_stdout, &_stderr, &exit);
    } catch (Glib::SpawnError &ex) {
      if (exit == 0) exit = -1;
      LOG (error) << "cmd: " << prefix << "failed to execute: '" << _cmd << "': " << ex.what ();
    }

    if (!_stdout.empty ()) {
      for (auto &l : VectorUtils::split_and_trim (_stdout, "\n")) {
        if (!l.empty ()) LOG (debug) << "cmd: " << prefix << l;
      }
    }

    if (!_stderr.empty ()) {
      for (auto &l : VectorUtils::split_and_trim (_stderr, "\n")) {
        if (!l.empty ()) LOG (error) << "cmd: " << prefix << l;
      }
    }

    return exit;
  }

  ustring Cmd::substitute (const ustring _cmd) {
    ustring ncmd = _cmd;

    ustring key = "hooks::";
    std::size_t fnd = ncmd.find (key);
    if (fnd != std::string::npos) {
      ncmd.replace (fnd, key.length (), (astroid->standard_paths ().config_dir / bfs::path("hooks/")).c_str ());
    }

    return ncmd;
  }
}


