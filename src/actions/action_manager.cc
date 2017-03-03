
# include <iostream>
# include <vector>

# include "astroid.hh"
# include "action_manager.hh"
# include "action.hh"
# include "db.hh"

using namespace std;

namespace Astroid {
  void ActionManager::doit (refptr<Action> action) {
    std::lock_guard<std::mutex> lk (actions_m);
    actions.push_back (action);
    actions_cv.notify_one ();
  }

  void ActionManager::doit (refptr<Action> action, bool undoable) {
    action->skip_undo = !undoable;
    doit (action);
  }

  void ActionManager::action_worker () {

    while (run) {
      std::unique_lock<std::mutex> lk (actions_m);
      actions_cv.wait (lk, [&] { return (!actions.empty () || !run); });

      /* lock emitter now, so that it does not start opening a
       * read-only db while the read-write db is open */
      lock_guard<std::mutex> elk (toemit_m);


      while (!actions.empty ()) {
        refptr<Action> a = actions.front ();
        actions.pop_front ();

        /* allow new actions to be queued while waiting for db */
        lk.unlock ();

        Db * db;
        std::unique_lock<std::mutex> rw_lock;

        if (a->need_db) {
          if (a->need_db_rw) {
            db = new Db (Db::DbMode::DATABASE_READ_WRITE);

          } else {
            db = new Db (Db::DbMode::DATABASE_READ_ONLY);

          }
        } else {
          if (a->need_db_rw) {
            rw_lock = Db::acquire_rw_lock ();
            db = NULL;
          } else {
            Db::acquire_ro_lock ();
            db = NULL;
          }
        }

        lk.lock ();

        if (!a->in_undo) {
          a->doit (db);
        } else {
          a->undo (db);
        }

        if (a->need_db) {
          db->close ();
          delete db;
        } else {
          if (a->need_db_rw) {
            Db::release_rw_lock (rw_lock);
          } else {
            Db::release_ro_lock ();
          }
        }

        if (!a->in_undo && a->undoable () && !a->skip_undo) {
          doneactions.push_back (a);
        }

        if (emit) toemit.push (a);
      }

      lk.unlock ();

      emit_ready ();
    }
  }

  void ActionManager::undo () {
    LOG (info) << "actions: undo";
    std::unique_lock<std::mutex> lk (actions_m);

    if (!actions.empty ()) {
      LOG (info) << "actions: action still in queue, removing..";

      /* get last action queued and remove before it is done */
      refptr<Action> a = actions.back ();
      if (!a->in_undo && !a->skip_undo) actions.pop_back ();

      /* just ignore the undo if the previous undo is not finished yet */
      return;

    } else {
      if (doneactions.empty ()) {
        LOG (debug) << "actions: no more actions to undo.";
        return;
      }

      LOG (info) << "actions: undoing already processed actions..";

      /* get last action added and undo it */
      refptr<Action> a = doneactions.back ();
      doneactions.pop_back ();

      a->in_undo = true;

      lk.unlock ();
      doit (a); // queue for undo
    }
  }

  void ActionManager::emitter () {
    /* runs on gui thread */
    if (emit) {
      while (!toemit.empty ()) {
        std::unique_lock<std::mutex> lk (toemit_m);

        refptr<Action> a = toemit.front ();
        toemit.pop ();
        lk.unlock ();

        Db db (Db::DATABASE_READ_ONLY);
        a->emit (&db);
      }
    }
  }

  ActionManager::ActionManager () {
    LOG (info) << "global actions: set up.";

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

  void ActionManager::close () {
    if (!run) return;

    LOG (debug) << "actions: cleaning up remaining actions..";
    emit = false;

    std::unique_lock<std::mutex> lk (actions_m);

    /* action worker are now waiting after the while (run) */
    run = false;
    lk.unlock ();
    actions_cv.notify_one ();
    action_worker_t.join ();
  }


  /* thread signals */
  ActionManager::type_signal_thread_updated
    ActionManager::signal_thread_updated ()
  {
    return m_signal_thread_updated;
  }

  void ActionManager::emit_thread_updated (Db * db, ustring thread_id) {
    LOG (info) << "actions: emitted updated and changed signal for thread: " << thread_id;
    m_signal_thread_updated.emit (db, thread_id);
    m_signal_thread_changed.emit (db, thread_id);
  }

  ActionManager::type_signal_thread_changed
    ActionManager::signal_thread_changed ()
  {
    return m_signal_thread_changed;
  }

  void ActionManager::emit_thread_changed (Db * db, ustring thread_id) {
    LOG (info) << "actions: emitted changed signal for thread: " << thread_id;
    m_signal_thread_changed.emit (db, thread_id);
  }

  /* message */
  ActionManager::type_signal_message_updated
    ActionManager::signal_message_updated ()
  {
    return m_signal_message_updated;
  }

  void ActionManager::emit_message_updated (Db * db, ustring message_id) {
    LOG (info) << "actions: emitted updated signal for message: " << message_id;
    m_signal_message_updated.emit (db, message_id);

    db->on_message (message_id,
        [&] (notmuch_message_t * nm_msg) {
          if (nm_msg != NULL) {
            const char * tidc = notmuch_message_get_thread_id (nm_msg);
            ustring tid;
            if (tidc != NULL) {
              tid = ustring(tidc);

              emit_thread_changed (db, tid);

            }
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
    LOG (info) << "actions: emitted refreshed signal.";
    m_signal_refreshed.emit ();
  }
}

