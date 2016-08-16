# include <mutex>
# include <chrono>

# include <boost/filesystem.hpp>
# include <glibmm/spawn.h>

# include "astroid.hh"
# include "poll.hh"
# include "db.hh"
# include "config.hh"
# include "actions/action_manager.hh"
# include "utils/vector_utils.hh"


using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  const int Poll::DEFAULT_POLL_INTERVAL = 60;

  Poll::Poll (bool _auto_polling_enabled) {
    LOG (info) << "poll: setting up.";

    auto_polling_enabled = _auto_polling_enabled;

    poll_state = false;

    poll_interval = astroid->config ().get<int> ("poll.interval");
    LOG (debug) << "poll: interval: " << poll_interval;

    // check every 1 seconds if periodic poll has changed
    Glib::signal_timeout ().connect (
        sigc::mem_fun (this, &Poll::periodic_polling), 1000);

    if (poll_interval <= 0) auto_polling_enabled = false;

    if (auto_polling_enabled) {
      // do initial poll
      poll ();

    } else {
      LOG (info) << "poll: periodic polling disabled.";
    }

    d_poll_state.connect (sigc::mem_fun (this, &Poll::poll_state_dispatch));
  }

  bool Poll::periodic_polling () {
    if (auto_polling_enabled) {
      chrono::duration<double> elapsed = chrono::steady_clock::now() - last_poll;

      if (elapsed.count () >= poll_interval) {
        LOG (info) << "poll: periodic poll..";
        poll ();
      }
    }

    return true;
  }

  void Poll::toggle_auto_poll () {
    LOG (info) << "poll: toggle auto poll: " << !auto_polling_enabled;

    if (poll_interval <= 0) {
      LOG (warn) << "poll: poll_interval = 0, setting to default: " << DEFAULT_POLL_INTERVAL;
      poll_interval = DEFAULT_POLL_INTERVAL;
    }

    auto_polling_enabled = !auto_polling_enabled;
  }

  bool Poll::poll () {
    LOG (debug) << "poll: requested..";

    // set this here as well to avoid lots of checks
    last_poll = chrono::steady_clock::now ();

    if (m_dopoll.try_lock ()) {

      Glib::Threads::Thread::create (
          sigc::mem_fun (this, &Poll::do_poll));

      return true;

    } else {
      LOG (warn) << "poll: already in progress.";

      return false;
    }

  }

  void Poll::do_poll () {
    set_poll_state (true);

    t0 = chrono::steady_clock::now ();

    path poll_script_uri = astroid->standard_paths().config_dir / path(poll_script);

    LOG (info) << "poll: polling: " << poll_script_uri.c_str ();

# ifdef HAVE_NOTMUCH_GET_REV
    Db db (Db::DbMode::DATABASE_READ_ONLY);
    before_poll_revision = db.get_revision ();
    LOG (debug) << "poll: revision before poll: " << before_poll_revision;
# endif

    if (!is_regular_file (poll_script_uri)) {
      LOG (error) << "poll: poll script does not exist or is not a regular file.";

      m_dopoll.unlock ();
      set_poll_state (false);
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
      LOG (error) << "poll: exception while running poll script: " <<  ex.what ();
      set_poll_state (false);
      m_dopoll.unlock ();
      return;
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
      LOG (error) << "poll: invalid fifo response";
    } else {
      Glib::ustring buf;

      ch_stdout->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      LOG (debug) << "poll script: " << buf;

    }
    return true;
  }

  bool Poll::log_err (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stderr.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      LOG (error) << "poll: invalid fifo response";
    } else {
      Glib::ustring buf;

      ch_stderr->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      LOG (warn) << "poll script: " << buf;
    }
    return true;
  }

  void Poll::child_watch (GPid pid, int child_status) {
    chrono::duration<double> elapsed = chrono::steady_clock::now() - t0;
    last_poll = chrono::steady_clock::now ();

    if (child_status != 0) {
      LOG (error) << "poll: poll script did not exit successfully.";
    }

    // TODO:
    // - use lastmod to figure out how many messages have been added or changed
    //   during poll.
    LOG (info) << "poll: done (time: " << elapsed.count() << " s) (child status: " << child_status << ")";
    set_poll_state (false);

    /* close process */
    Glib::spawn_close_pid (pid);

    if (child_status == 0) {

# ifdef HAVE_NOTMUCH_GET_REV

      /* first poll */
      if (last_good_before_poll_revision == 0)
        last_good_before_poll_revision = before_poll_revision;

      /* update all threads that have been changed */
      Db db (Db::DbMode::DATABASE_READ_ONLY);

      unsigned long revnow = db.get_revision ();
      LOG (debug) << "poll: revision after poll: " << revnow;

      if (revnow > last_good_before_poll_revision) {

        ustring query = ustring::compose ("lastmod:%1..%2",
            before_poll_revision,
            revnow);

        notmuch_query_t * qry = notmuch_query_create (db.nm_db, query.c_str ());

        unsigned int total_threads;
        notmuch_status_t st = NOTMUCH_STATUS_SUCCESS;
# ifdef HAVE_QUERY_COUNT_THREADS_ST
        st = notmuch_query_count_threads_st (qry, &total_threads);
# else
        total_threads = notmuch_query_count_threads (qry);
# endif

        LOG (info) << "poll: " << total_threads << " threads changed, updating..";

        if (st == NOTMUCH_STATUS_SUCCESS && total_threads > 0) {
          notmuch_threads_t * threads;
          notmuch_thread_t  * thread;
# ifdef HAVE_QUERY_THREADS_ST
          st = notmuch_query_search_threads_st (qry, &threads);
# else
          threads = notmuch_query_search_threads (qry);
# endif

          for (;
               (st == NOTMUCH_STATUS_SUCCESS) && notmuch_threads_valid (threads);
               notmuch_threads_move_to_next (threads)) {

            thread = notmuch_threads_get (threads);

            const char * t = notmuch_thread_get_thread_id (thread);

            ustring tt (t);
            astroid->actions->emit_thread_updated (&db, tt);
          }
        }

        notmuch_query_destroy (qry);

      }

      last_good_before_poll_revision = revnow;
# else
      astroid->actions->signal_refreshed_dispatcher ();
# endif
    }

    m_dopoll.unlock ();
  }

  void Poll::poll_state_dispatch () {
    emit_poll_state (poll_state);
  }

  Poll::type_signal_poll_state
    Poll::signal_poll_state ()
  {
    return m_signal_poll_state;
  }

  void Poll::emit_poll_state (bool state) {
    LOG (info) << "poll: emitted poll state: " << state;

    m_signal_poll_state.emit (state);
  }

  void Poll::set_poll_state (bool state) {
    if (state != poll_state) {
      poll_state = state;
      d_poll_state.emit ();
    }
  }

  bool Poll::get_poll_state () {
    return poll_state;
  }
}

