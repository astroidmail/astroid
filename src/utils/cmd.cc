# include "cmd.hh"
# include "astroid.hh"
# include "log.hh"
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


