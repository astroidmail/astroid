
# include <iostream>
# include <vector>

# include "astroid.hh"
# include "action_manager.hh"
# include "main_window.hh"
# include "action.hh"
# include "db.hh"
# include "log.hh"

using namespace std;

namespace Astroid {
  ActionManager::ActionManager () {

  }

  bool ActionManager::doit (Db * db, refptr<Action> action) {
    if (action->undoable()) {
      actions.push_back (action);
    }
    return action->doit (db);
  }

  bool ActionManager::undo () {
    log << info << "actions: undo" << endl;
    if (!actions.empty ()) {
      refptr<Action> action = actions.back ();
      actions.pop_back ();
      Db db (Db::DbMode::DATABASE_READ_WRITE);
      lock_guard<Db> grd (db);
      return action->undo (&db);
    } else {
      log << info << "actions: no more actions to undo." << endl;
      return true;
    }
  }
}

