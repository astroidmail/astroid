
# include <iostream>
# include <vector>

# include "astroid.hh"
# include "action_manager.hh"
# include "action.hh"
# include "db.hh"
# include "log.hh"

using namespace std;

namespace Astroid {
  ActionManager::ActionManager () {

  }

  bool ActionManager::doit (Db * db, refptr<Action> action) {
    if (action->undoable()) {
      actions.push_back (action);
    }

    bool res = action->doit (db);

    if (res) {
      action->emit (db);
    }

    return res;
  }

  bool ActionManager::undo () {
    log << info << "actions: undo" << endl;
    if (!actions.empty ()) {
      refptr<Action> action = actions.back ();
      actions.pop_back ();
      Db db (Db::DbMode::DATABASE_READ_WRITE);
      lock_guard<Db> grd (db);

      bool res = action->undo (&db);

      if (res) {
        action->emit (&db);
      }

      return res;
    } else {
      log << info << "actions: no more actions to undo." << endl;
      return true;
    }
  }

  GlobalActions::GlobalActions () {
    log << info << "global actions: set up." << endl;

    /* set up GUI thread dispatcher for out-of-thread
     * signals */
    signal_refreshed_dispatcher.connect (
        sigc::mem_fun (this,
          &GlobalActions::emit_refreshed));
  }

  /* signals */
  GlobalActions::type_signal_thread_updated
    GlobalActions::signal_thread_updated ()
  {
    return m_signal_thread_updated;
  }

  void GlobalActions::emit_thread_updated (Db * db, ustring thread_id) {
    log << info << "actions: emitted updated signal for thread: " << thread_id << endl;
    m_signal_thread_updated.emit (db, thread_id);
  }

  GlobalActions::type_signal_refreshed
    GlobalActions::signal_refreshed ()
  {
    return m_signal_refreshed;
  }

  void GlobalActions::emit_refreshed () {
    log << info << "actions: emitted refreshed signal." << endl;
    m_signal_refreshed.emit ();
  }
}

