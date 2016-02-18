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

      virtual void emit (Db *) = 0;
  };
}
