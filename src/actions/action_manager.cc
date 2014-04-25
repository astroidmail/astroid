# include "action_manager.hh"

# include <iostream>
# include <vector>

# include "astroid.hh"
# include "action.hh"

using namespace std;

namespace Astroid {
  ActionManager::ActionManager () {


  }

  bool ActionManager::doit (refptr<Action> action) {
    if (action->undoable()) {
      actions.push_back (action);
    }
    return action->doit ();
  }

  bool ActionManager::undo () {
    cout << "actions: undo" << endl;
    if (!actions.empty ()) {
      refptr<Action> action = actions.back ();
      actions.pop_back ();
      return action->undo ();
    } else {
      cout << "actions: no more actions to undo." << endl;
      return true;
    }
  }
}

