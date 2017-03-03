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
      /* Overview of signals:
       *
       * thread-updated, thread-changed and message-updated are called as
       * based on user or external actions.
       *
       * Db::NotmuchMessage or Db::NotmuchThread will not be respond to these
       * signals, they must be updated manually be anyone keeping track of them.
       *
       * Message and MessageThread do respond to these changes and update themselves
       * if necessary. Message emits a message-changed signal so that any user of
       * open Messages can be notified _after_ the message has been updated from
       * the Db. In the future there might be a 'message-thread-changed' signal
       * emitted from MessageThread.
       *
       * MessageThread updates all its messages if a 'thread-updated' signal is
       * emitted, if 'thread-changed' is emitted it only updates the generic
       * thread properties.
       *
       * This means that the thread-index which only keeps Notmuch* objects must
       * updated them manually, while the thread-view which only keeps Message*
       * objects rely on the signals sent by the Message* objects so that it is
       * certain that the Message* objects have been refreshed when it gets the
       * signal.
       *
       */

      /* thread-updated: emitted from e.g. thread-index and poll, but
       * not from message changes - so suitable for changes where there
       * is _not_ a redundant message_updated signal and all messages
       * _should_ also be updated */
      typedef sigc::signal <void, Db *, ustring> type_signal_thread_updated;
      type_signal_thread_updated signal_thread_updated ();

      void emit_thread_updated (Db *, ustring);

      /* thread-changed: more restrictive than thread-updated. emitted from
       * message-updated, as well as from thread-updated. so suitable for
       * thread-index where message-updated events are handled or not needed
       * and messages contained in the thread _should not_ be updated. */
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
