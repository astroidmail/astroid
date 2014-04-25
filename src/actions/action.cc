# include <iostream>

# include "astroid.hh"
# include "db.hh"

# include "action.hh"

using namespace std;

namespace Astroid {
  Action::Action (refptr<NotmuchThread> nmt) {

    thread = nmt;

  }

  bool Action::undoable () {
    return false;
  }

}

