
# include <iostream>
# include <vector>

# include "astroid.hh"
# include "action_manager.hh"
# include "action.hh"
# include "db.hh"
# include "log.hh"

using namespace std;

namespace Astroid {
  void ActionManager::doit (refptr<Action> action) {
    std::lock_guard<std::mutex> lk (actions_m);
    actions.push_back (action);
    actions_cv.notify_one ();
  }

  void ActionManager::action_worker () {

    while (run) {
      std::unique_lock<std::mutex> lk (actions_m);
      actions_cv.wait (lk, [&] { return !actions.empty (); });

      /* lock emitter now, so that it does not start opening a
       * read-only db while the read-write db is open */
      lock_guard<std::mutex> elk (toemit_m);


      while (!actions.empty ()) {
        refptr<Action> a = actions.front ();
        actions.pop_front ();

        /* allow new actions to be queued while waiting for db */
        lk.unlock ();

        Db db (a->needrwdb ? Db::DbMode::DATABASE_READ_WRITE : Db::DbMode::DATABASE_READ_ONLY);

        lk.lock ();

        if (!a->in_undo) {
          a->doit (&db);
        } else {
          a->undo (&db);
        }

        db.close ();

        if (!a->in_undo && a->undoable ()) {
          doneactions.push_back (a);
        }

        toemit.push (a);
      }

      lk.unlock ();

      emit_ready ();
    }
  }

  void ActionManager::undo () {
    log << info << "actions: undo" << endl;
    std::lock_guard<std::mutex> lk (actions_m);

    if (!actions.empty ()) {
      log << info << "actions: action still in queue, removing.." << endl;

      /* get last action queued and remove before it is done */
      refptr<Action> a = actions.back ();
      if (!a->in_undo) actions.pop_back ();

      /* just ignore the undo if the previous undo is not finished yet */
      return;

    } else {
      if (doneactions.empty ()) {
        log << "actions: no more actions to undo." << endl;
        return;
      }

      log << info << "actions: undoing already processed actions.." << endl;

      /* get last action added and undo it */
      refptr<Action> a = doneactions.back ();
      doneactions.pop_back ();

      a->in_undo = true;
      doit (a); // queue for undo
    }
  }

  void ActionManager::emitter () {
    /* runs on gui thread */
    std::lock_guard<std::mutex> lk (toemit_m);
    Db db (Db::DATABASE_READ_ONLY);
    while (!toemit.empty ()) {
      refptr<Action> a = toemit.front ();
      toemit.pop ();

      a->emit (&db);
    }
  }

  ActionManager::ActionManager () {
    log << info << "global actions: set up." << endl;

    /* set up GUI thread dispatcher for out-of-thread
     * signals */
    signal_refreshed_dispatcher.connect (
        sigc::mem_fun (this,
          &ActionManager::emit_refreshed));

    emit_ready.connect (
        sigc::mem_fun (this,
          &ActionManager::emitter));

    run = true;
    action_worker_t = std::thread (&ActionManager::action_worker, this);
  }

  ActionManager::~ActionManager () {
    run = false;
    action_worker_t.join ();
  }

  /* signals */
  ActionManager::type_signal_thread_updated
    ActionManager::signal_thread_updated ()
  {
    return m_signal_thread_updated;
  }

  void ActionManager::emit_thread_updated (Db * db, ustring thread_id) {
    log << info << "actions: emitted updated and changed signal for thread: " << thread_id << endl;
    m_signal_thread_updated.emit (db, thread_id);
    m_signal_thread_changed.emit (db, thread_id);
  }

  ActionManager::type_signal_thread_changed
    ActionManager::signal_thread_changed ()
  {
    return m_signal_thread_changed;
  }

  void ActionManager::emit_thread_changed (Db * db, ustring thread_id) {
    log << info << "actions: emitted changed signal for thread: " << thread_id << endl;
    m_signal_thread_changed.emit (db, thread_id);
  }

  /* message */
  ActionManager::type_signal_message_updated
    ActionManager::signal_message_updated ()
  {
    return m_signal_message_updated;
  }

  void ActionManager::emit_message_updated (Db * db, ustring message_id) {
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
  ActionManager::type_signal_refreshed
    ActionManager::signal_refreshed ()
  {
    return m_signal_refreshed;
  }

  void ActionManager::emit_refreshed () {
    log << info << "actions: emitted refreshed signal." << endl;
    m_signal_refreshed.emit ();
  }
}

