
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

    action->emit (db);

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

      action->emit (&db);

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
    log << info << "actions: emitted updated and changed signal for thread: " << thread_id << endl;
    m_signal_thread_updated.emit (db, thread_id);
    m_signal_thread_changed.emit (db, thread_id);
  }

  GlobalActions::type_signal_thread_changed
    GlobalActions::signal_thread_changed ()
  {
    return m_signal_thread_changed;
  }

  void GlobalActions::emit_thread_changed (Db * db, ustring thread_id) {
    log << info << "actions: emitted changed signal for thread: " << thread_id << endl;
    m_signal_thread_changed.emit (db, thread_id);
  }

  /* message */
  GlobalActions::type_signal_message_updated
    GlobalActions::signal_message_updated ()
  {
    return m_signal_message_updated;
  }

  void GlobalActions::emit_message_updated (Db * db, ustring message_id) {
    log << info << "actions: emitted updated signal for message: " << message_id << endl;
    m_signal_message_updated.emit (db, message_id);

    db->on_message (message_id,
        [&] (notmuch_message_t * nm_msg) {
          const char * tidc = notmuch_message_get_thread_id (nm_msg);
          ustring tid;
          if (tidc != NULL) {
            tid = ustring(tidc);

            emit_thread_changed (db, tid);

          }
      });
  }

  /* refreshed */
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

