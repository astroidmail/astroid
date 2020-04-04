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
  Cmd::Cmd (ustring _prefix, ustring _cmd, ustring _undo_cmd) : Cmd (_cmd, _undo_cmd) {
    prefix = _prefix + ": ";
  }

  Cmd::Cmd (ustring _cmd, ustring _undo_cmd) {
    cmd = substitute (_cmd);
    undo_cmd = substitute (_undo_cmd);
  }

  Cmd::Cmd () {

  }

  int Cmd::run () {
    return execute (false);
  }

  bool Cmd::undoable () {
    return !undo_cmd.empty ();
  }

  int Cmd::undo () {
    if (!undoable ()) {
      LOG (error) << "cmd: tried to undo non-undoable command: " << cmd;
      return 1;
    }
    return execute (true);
  }

  int Cmd::execute (bool undo) {
    ustring c = (!undo ? cmd : undo_cmd);

    LOG (info) << "cmd: running: " << c;
    int exit;
    string _stdout, _stderr;

    string _cmd = c;
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

    return (exit == 0);
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

  bool Cmd::pipe (ustring cmd, const ustring& _stdin, ustring& _stdout, ustring &_stderr) {
    LOG (info) << "cmd: running: " << cmd;

    try {
      int pid;
      int stdin;
      int stdout;
      int stderr;
      std::vector<std::string> args = Glib::shell_parse_argv (cmd);

      Glib::spawn_async_with_pipes ("",
          args,
          Glib::SPAWN_DO_NOT_REAP_CHILD |
          Glib::SPAWN_SEARCH_PATH,
          sigc::slot <void> (),
          &pid,
          &stdin,
          &stdout,
          &stderr
          );

      refptr<Glib::IOChannel> ch_stdin;
      refptr<Glib::IOChannel> ch_stdout;
      refptr<Glib::IOChannel> ch_stderr;
      ch_stdin  = Glib::IOChannel::create_from_fd (stdin);
      ch_stdout = Glib::IOChannel::create_from_fd (stdout);
      ch_stderr = Glib::IOChannel::create_from_fd (stderr);

      ch_stdin->write (_stdin);
      ch_stdin->close ();

      ch_stderr->read_to_end (_stderr);
      ch_stderr->close ();

      if (!_stderr.empty ()) {
        LOG (error) << "cmd: " << _stderr;
      }

      ch_stdout->read_to_end (_stdout);
      ch_stdout->close ();

      g_spawn_close_pid (pid);

    } catch (Glib::SpawnError &ex) {
      LOG (error) << "cmd: failed to execute: '" << cmd << "': " << ex.what ();
      return false;
    }


    return true;
  }
}


