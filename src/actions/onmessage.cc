# include <functional>
# include <vector>

# include <notmuch.h>

# include "onmessage.hh"
# include "db.hh"
# include "action_manager.hh"
# include "astroid.hh"

namespace Astroid {
  OnMessageAction::OnMessageAction (
      ustring _msg_id,
      ustring _tid,
      std::function <void(Db *, notmuch_message_t *)> _b) {

    msg_id  = _msg_id;
    tid     = _tid;
    block   = _b;
  }

  bool OnMessageAction::doit (Db * db) {
    db->on_message (msg_id, std::bind (block, db, std::placeholders::_1));
    return true;
  }

  bool OnMessageAction::undo (Db *) {
    return false;
  }

  bool OnMessageAction::undoable () {
    return false;
  }

  void OnMessageAction::emit (Db * db) {
    astroid->actions->emit_message_updated (db, msg_id);

    if (tid != "")
      astroid->actions->emit_thread_updated (db, tid);
  }


  AddDraftMessage::AddDraftMessage (ustring _f) {
    fname = _f;
  }

  bool AddDraftMessage::doit (Db * db) {
    mid = db->add_draft_message (fname);
    return true;
  }

  bool AddDraftMessage::undo (Db *) {
    return false;
  }

  bool AddDraftMessage::undoable () {
    return false;
  }

  void AddDraftMessage::emit (Db * db) {
    if (mid != "")
      astroid->actions->emit_message_updated (db, mid);
  }

  AddSentMessage::AddSentMessage (ustring _f, std::vector<ustring> _additional_sent_tags) {
    fname = _f;
    additional_sent_tags = _additional_sent_tags;
  }

  bool AddSentMessage::doit (Db * db) {
    mid = db->add_sent_message (fname, additional_sent_tags);
    return true;
  }

  bool AddSentMessage::undo (Db *) {
    return false;
  }

  bool AddSentMessage::undoable () {
    return false;
  }

  void AddSentMessage::emit (Db * db) {
    if (mid != "")
      astroid->actions->emit_message_updated (db, mid);
  }
}

