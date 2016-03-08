# pragma once

# include "action.hh"
# include "utils/cmd.hh"

namespace Astroid {
  class CmdAction : public Action {
    public:
      CmdAction (Cmd c, ustring thread_id, ustring mid);


      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;
      virtual void emit (Db *) override;

    private:
      Cmd cmd;
      ustring thread_id;
      ustring mid;

  };
}
