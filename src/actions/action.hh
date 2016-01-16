# pragma once

# include <glibmm.h>
# include <vector>

# include "proto.hh"

namespace Astroid {
  class Action : public Glib::Object {
    public:
      Action (refptr<NotmuchThread>);
      Action (std::vector<refptr<NotmuchThread>>);

      std::vector<refptr<NotmuchThread>> threads;

      virtual bool doit (Db *) = 0;
      virtual bool undo (Db *) = 0;
      virtual bool undoable ();

      virtual void emit (Db *);
  };
}
