# pragma once

# include <vector>

# include <sigc++/sigc++.h>
# include <glibmm/threads.h>

# include "proto.hh"

namespace Astroid {
  class ActionManager {
    public:
      ActionManager ();

      std::vector<refptr<Action>> actions;

      bool doit (Db *, refptr<Action>);
      bool undo ();
  };

  class GlobalActions {
    public:
      GlobalActions ();

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
