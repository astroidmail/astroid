# pragma once

# include <vector>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class UndoManager {
    public:
      UndoManager ();

      vector<refptr<Action>> actions;

  };
}
