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
  Poll::Poll () {
    log << info << "poll: setting up." << endl;
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
    chrono::time_point<chrono::steady_clock> t0 = chrono::steady_clock::now ();

    path poll_script_uri = astroid->config->config_dir / path(poll_script);

    log << info << "poll: polling: " << poll_script_uri.c_str () << endl;

    if (!is_regular_file (poll_script_uri)) {
      log << error << "poll: poll script does not exist or is not a regular file." << endl;

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
      log << error << "poll: exception while running poll script: " <<  ex.what () << endl;
    }

    ustring ustdout = ustring(stdout);
    for (ustring &l : VectorUtils::split_and_trim (ustdout, ustring("\n"))) {

      log << debug << l << endl;
    }

    ustring ustderr = ustring(stderr);
    for (ustring &l : VectorUtils::split_and_trim (ustderr, ustring("\n"))) {

      log << debug << l << endl;
    }

    if (exitcode != 0) {
      log << error << "poll: poll script exited with code: " << exitcode << endl;

    }

    chrono::duration<double> elapsed = chrono::steady_clock::now() - t0;

    // TODO:
    // - use lastmod to figure out how many messages have been added or changed
    //   during poll.
    log << info << "poll: done (time: " << elapsed.count() << " s)." << endl;

    astroid->global_actions->signal_refreshed_dispatcher ();

    m_dopoll.unlock ();
  }

}

