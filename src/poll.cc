# include <mutex>
# include <thread>
# include <chrono>

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
  Poll::Poll (bool auto_polling_enabled) {
    log << info << "poll: setting up." << endl;

    poll_interval = astroid->config->config.get<int> ("poll.interval");
    log << debug << "poll: interval: " << poll_interval << endl;

    if (auto_polling_enabled && poll_interval > 0) {

      Glib::signal_timeout ().connect (
          sigc::mem_fun (this, &Poll::periodic_polling), poll_interval * 1000);

      // do initial poll
      poll ();

    } else {
      log << info << "poll: periodic polling disabled." << endl;
    }
  }

  bool Poll::periodic_polling () {
    chrono::duration<double> elapsed = chrono::steady_clock::now() - last_poll;

    if (elapsed.count () >= poll_interval) {
      log << info << "poll: periodic poll.." << endl;
      poll ();
    }

    return true;
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
    t0 = chrono::steady_clock::now ();

    path poll_script_uri = astroid->config->config_dir / path(poll_script);

    log << info << "poll: polling: " << poll_script_uri.c_str () << endl;

    if (!is_regular_file (poll_script_uri)) {
      log << error << "poll: poll script does not exist or is not a regular file." << endl;

      m_dopoll.unlock ();
      return;
    }

    vector<string> args = {poll_script_uri.c_str()};
    try {
      Glib::spawn_async_with_pipes ("",
                        args,
                        Glib::SPAWN_DO_NOT_REAP_CHILD,
                        sigc::slot <void> (),
                        &pid,
                        &stdin,
                        &stdout,
                        &stderr
                        );
    } catch (Glib::SpawnError &ex) {
      log << error << "poll: exception while running poll script: " <<  ex.what () << endl;
    }

    /* connect channels */
    Glib::signal_io().connect (sigc::mem_fun (this, &Poll::log_out), stdout, Glib::IO_IN | Glib::IO_HUP);
    Glib::signal_io().connect (sigc::mem_fun (this, &Poll::log_err), stderr, Glib::IO_IN | Glib::IO_HUP);
    Glib::signal_child_watch().connect (sigc::mem_fun (this, &Poll::child_watch), pid);

    ch_stdout = Glib::IOChannel::create_from_fd (stdout);
    ch_stderr = Glib::IOChannel::create_from_fd (stderr);
  }

  bool Poll::log_out (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stdout.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      log << error << "poll: invalid fifo response" << endl;
    } else {
      Glib::ustring buf;

      ch_stdout->read_line(buf);
      buf.erase (--buf.end());

      log << debug << "poll script: " << buf << endl;

    }
    return true;
  }

  bool Poll::log_err (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stderr.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      log << error << "poll: invalid fifo response" << endl;
    } else {
      Glib::ustring buf;

      ch_stderr->read_line(buf);
      buf.erase (--buf.end());

      log << warn << "poll script: " << buf << endl;
    }
    return true;
  }

  void Poll::child_watch (GPid pid, int child_status) {
    chrono::duration<double> elapsed = chrono::steady_clock::now() - t0;
    last_poll = chrono::steady_clock::now ();

    if (child_status != 0) {
      log << error << "poll: poll script did not exit successfully." << endl;
    }

    // TODO:
    // - use lastmod to figure out how many messages have been added or changed
    //   during poll.
    log << info << "poll: done (time: " << elapsed.count() << " s) (child status: " << child_status << ")" << endl;

    /* close process */
    Glib::spawn_close_pid (pid);

    astroid->global_actions->signal_refreshed_dispatcher ();

    m_dopoll.unlock ();
  }
}

