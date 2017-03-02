# include "astroid.hh"
# include "action_manager.hh"
# include "cmdaction.hh"

namespace Astroid {
  CmdAction::CmdAction (Cmd _c, ustring _tid, ustring _mid) {
    cmd = _c;
    thread_id = _tid;
    mid = _mid;
    need_db    = false;
    need_db_rw = true;
  }

  bool CmdAction::doit (Db *) {
    return cmd.run ();
  }

  bool CmdAction::undo (Db *) {
    return false;
  }

  bool CmdAction::undoable () {
    return false;
  }

  void CmdAction::emit (Db * db) {
    /* if there is a thread_id given: we cannot know if only the thread or only
     * the messages has been modified in the hook. we therefore always have to
     * emit a full 'thread-update' signal. this _should_ cause all messages in
     * the thread, including the one passed as mid, to be updated. emitting a
     * second 'messsage-updated' is therefore redundant in these cases.
     */

    if (thread_id != "") {
      // will also cause all messages in thread to be updated
      astroid->actions->emit_thread_updated (db, thread_id);
      return;
    }

    if (mid != "") {
      // will also emit thread_changed
      astroid->actions->emit_message_updated (db, mid);
    }
  }
}

