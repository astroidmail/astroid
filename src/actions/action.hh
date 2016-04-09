# pragma once

# include <glibmm.h>
# include <vector>

# include "proto.hh"

namespace Astroid {
  class Action : public Glib::Object {
    friend class ActionManager;

    public:
      virtual bool doit (Db *) = 0;
      virtual bool undo (Db *) = 0;
      virtual bool undoable ();

      virtual void emit (Db *) = 0;

    protected:
      /* used when undoing, the action_worker will undo the action
       * without adding it to the doneactions */
      bool in_undo = false;

      /* has the same effect as in_undo, but allows a stanard undoable
       * action to be queued without keeping putting it in the undo queue */
      bool skip_undo = false;

      /* generally actions need a read-write db */
      bool need_db    = true; /* only this will give ro-db */
      bool need_db_rw = true; /* only this will lock db-rw */
  };
}
