# pragma once

# include <vector>

# include <sigc++/sigc++.h>
# include <glibmm/threads.h>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class ActionManager {
    public:
      ActionManager ();

      vector<refptr<Action>> actions;

      bool doit (Db *, refptr<Action>);
      bool undo ();
  };

  class GlobalActions {
    public:
      GlobalActions ();

      /* update signal (something has been done with a thread) */
      typedef sigc::signal <void, Db *, ustring> type_signal_thread_updated;
      type_signal_thread_updated signal_thread_updated ();

      void emit_thread_updated (Db *, ustring);

      /* refresh signal (after polling) */
      typedef sigc::signal <void> type_signal_refreshed;
      type_signal_refreshed signal_refreshed ();

      void emit_refreshed ();

      Glib::Dispatcher signal_refreshed_dispatcher;

    protected:
      type_signal_thread_updated m_signal_thread_updated;
      type_signal_refreshed m_signal_refreshed;

  };
}
