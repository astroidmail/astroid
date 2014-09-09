# include <mutex>
# include <thread>

# include <boost/filesystem.hpp>
# include <glibmm/spawn.h>

# include "astroid.hh"
# include "poll.hh"
# include "log.hh"
# include "config.hh"
# include "actions/action_manager.hh"
# include "utils/vector_utils.hh"


using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  Poll::Poll () {
    log << info << "poll: setting up." << endl;

    signal_log.connect (sigc::mem_fun (this, &Poll::flush_log_lines));
  }

  void Poll::flush_log_lines () {
    lock_guard<mutex> grd (m_log_lines);

    for (LogLine &l : log_lines) {
      log << l.lvl << l.str << endl;
    }

    log_lines.clear ();
  }

  bool Poll::poll () {
    log << debug << "poll: requested.." << endl;

    if (m_dopoll.try_lock ()) {

      thread job (&Poll::do_poll, this);
      job.detach ();

      return true;

    } else {
      log << warn << "poll: already in progress." << endl;

      return false;
    }

  }

  void Poll::do_poll () {
    lastpoll = clock ();

    path poll_script_uri = astroid->config->config_dir / path(poll_script);

    stringstream str;
    str << "poll: polling: " << poll_script_uri.c_str ();

    m_log_lines.lock ();
    log_lines.push_back (LogLine (info, str.str()));
    m_log_lines.unlock ();
    signal_log ();

    str.str("");

    if (!is_regular_file (poll_script_uri)) {
      m_log_lines.lock ();
      log_lines.push_back (LogLine (error, "poll: poll script does not exist or is not a regular file."));
      m_log_lines.unlock ();
      signal_log ();

      m_dopoll.unlock ();
      return;
    }

    vector<string> args = {poll_script_uri.c_str()};
    string stdout;
    string stderr;
    int    exitcode;
    try {
      Glib::spawn_sync ("",
                        args,
                        Glib::SPAWN_DEFAULT,
                        sigc::slot <void> (),
                        &stdout,
                        &stderr,
                        &exitcode
                        );
    } catch (Glib::SpawnError &ex) {
      str.str ("poll: exception while running poll script: ");
      str << ex.what ();

      m_log_lines.lock ();
      log_lines.push_back (LogLine (error, str.str()));
      m_log_lines.unlock ();
      signal_log ();
    }

    ustring ustdout = ustring(stdout);
    for (ustring &l : VectorUtils::split_and_trim (ustdout, ustring("\n"))) {

      m_log_lines.lock ();
      log_lines.push_back (LogLine (debug, l));
      m_log_lines.unlock ();
      signal_log ();

    }

    ustring ustderr = ustring(stderr);
    for (ustring &l : VectorUtils::split_and_trim (ustderr, ustring("\n"))) {

      m_log_lines.lock ();
      log_lines.push_back (LogLine (debug, l));
      m_log_lines.unlock ();
      signal_log ();

    }

    if (exitcode != 0) {
      str.str ("poll: poll script exited with code: ");
      str << exitcode;

      m_log_lines.lock ();
      log_lines.push_back (LogLine (error, str.str()));
      m_log_lines.unlock ();
      signal_log ();
    }

    str.str ("");
    str << "poll: done (time: " << ((clock() - lastpoll) * 1000.0 / CLOCKS_PER_SEC) << " ms).";

    m_log_lines.lock ();
    log_lines.push_back (LogLine (info, str.str()));
    m_log_lines.unlock ();
    signal_log ();

    astroid->global_actions->signal_refreshed_dispatcher ();

    m_dopoll.unlock ();
  }

}

