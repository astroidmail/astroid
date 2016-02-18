# include <iostream>

# include "astroid.hh"
# include "db.hh"

# include "action_manager.hh"
# include "action.hh"

using namespace std;

namespace Astroid {
  bool Action::undoable () {
    return false;
  }

}

