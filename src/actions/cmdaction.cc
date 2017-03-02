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
    if (mid != "") {
      // will also emit thread_changed
      astroid->actions->emit_message_updated (db, mid);
      return;
    }

    if (thread_id != "") {
      astroid->actions->emit_thread_updated (db, thread_id);
    }
  }
}
