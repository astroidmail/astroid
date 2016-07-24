# pragma once

# include <vector>
# include <queue>
# include <deque>
# include <thread>
# include <mutex>
# include <condition_variable>

# include <sigc++/sigc++.h>

# include "proto.hh"

namespace Astroid {
  class ActionManager {
    public:
      ActionManager ();

      void doit (refptr<Action>);
      void doit (refptr<Action>, bool);
      void undo ();
      void close ();

    private:
      bool run = false;
      std::thread action_worker_t;
      void action_worker ();

      std::mutex actions_m;
      std::condition_variable actions_cv;

      std::mutex toemit_m;

      std::deque<refptr<Action>> doneactions;
      std::deque<refptr<Action>> actions;
      std::queue<refptr<Action>> toemit;

      Glib::Dispatcher emit_ready;
      void emitter ();

      /* used when closing: do not emit signals when closing */
      bool emit = true;

    public:

      /* thread updated: called from e.g. thread-index and poll, but
       * not from message changes - so suitable for changes where there
       * is _not_ an redundant message_updated signal. */
      typedef sigc::signal <void, Db *, ustring> type_signal_thread_updated;
      type_signal_thread_updated signal_thread_updated ();

      void emit_thread_updated (Db *, ustring);

      /* thread changed: called from message updated, as well as those above.
       * so suitable for thread-index where message_updated events are not
       * handled. */
      typedef sigc::signal <void, Db *, ustring> type_signal_thread_changed;
      type_signal_thread_changed signal_thread_changed ();

      void emit_thread_changed (Db *, ustring);

      /* message update signal */
      typedef sigc::signal <void, Db *, ustring> type_signal_message_updated;
      type_signal_message_updated signal_message_updated ();

      void emit_message_updated (Db *, ustring);

      /* refresh signal (after polling) */
      typedef sigc::signal <void> type_signal_refreshed;
      type_signal_refreshed signal_refreshed ();

      void emit_refreshed ();

      Glib::Dispatcher signal_refreshed_dispatcher;

    protected:
      type_signal_thread_updated m_signal_thread_updated;
      type_signal_thread_changed m_signal_thread_changed;
      type_signal_message_updated m_signal_message_updated;
      type_signal_refreshed m_signal_refreshed;

  };
}
