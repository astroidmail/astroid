# pragma once

# include "astroid.hh"

# include <mutex>
# include <chrono>
# include <glibmm/threads.h>
# include <glibmm/iochannel.h>

namespace Astroid {
  class Poll {
    public:
      Poll (bool auto_polling_enabled);

      bool poll ();
      void toggle_auto_poll ();
      bool get_auto_poll ();

      const char * poll_script = "poll.sh";
      static const int DEFAULT_POLL_INTERVAL; // 60

      void start_polling ();
      void stop_polling ();
      void refresh (unsigned long before);

    private:
      std::mutex m_dopoll;

      int poll_interval = 0;
      bool auto_polling_enabled = true;
      bool external_polling = false;

      void do_poll ();
      bool periodic_polling ();

      std::chrono::time_point<std::chrono::steady_clock> t0; // start time of poll
      std::chrono::time_point<std::chrono::steady_clock> last_poll;

# ifdef HAVE_NOTMUCH_GET_REV
      unsigned long before_poll_revision = 0;
# endif
      void refresh_threads ();

      int pid;
      int stdin;
      int stdout;
      int stderr;
      refptr<Glib::IOChannel> ch_stdout;
      refptr<Glib::IOChannel> ch_stderr;

      bool log_out (Glib::IOCondition);
      bool log_err (Glib::IOCondition);
      void child_watch (GPid, int);

      bool poll_state;
      void set_poll_state (bool);
      Glib::Dispatcher d_poll_state;
      void poll_state_dispatch ();

    public:
      bool get_poll_state ();

      /* poll state change signal */
      typedef sigc::signal <void, bool> type_signal_poll_state;
      type_signal_poll_state signal_poll_state ();

      void emit_poll_state (bool);
    protected:
      type_signal_poll_state m_signal_poll_state;
  };
}

