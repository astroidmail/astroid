# pragma once

# include <glibmm.h>
# include <vector>

# include "proto.hh"

namespace Astroid {
  class Action : public Glib::Object {
    public:
      virtual bool doit (Db *) = 0;
      virtual bool undo (Db *) = 0;
      virtual bool undoable ();

      /* used when undoing, the action_worker will undo the action
       * without adding it to the doneactions */
      bool in_undo = false;

      virtual void emit (Db *) = 0;
  };
}
