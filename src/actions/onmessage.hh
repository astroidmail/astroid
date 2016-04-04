# pragma once

# include <vector>
# include <functional>

# include <notmuch.h>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class OnMessageAction : public Action {
    public:
      OnMessageAction (ustring msg_id, ustring tid, std::function <void(Db *, notmuch_message_t *)>);

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;
      virtual void emit (Db *) override;

    private:
      ustring msg_id;
      ustring tid;
      std::function <void(Db *, notmuch_message_t *)> block;

  };

  class AddDraftMessage : public Action {
    public:
      AddDraftMessage (ustring fname);

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;
      virtual void emit (Db *) override;

    private:
      ustring fname;
      ustring mid;
  };

  class AddSentMessage : public Action {
    public:
      AddSentMessage (ustring fname, std::vector<ustring> additional_sent_tags);

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;
      virtual void emit (Db *) override;

    private:
      ustring fname;
      ustring mid;
      std::vector<ustring> additional_sent_tags;
  };

}
