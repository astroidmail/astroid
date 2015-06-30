# include <iostream>

# include "astroid.hh"
# include "db.hh"

# include "action_manager.hh"
# include "action.hh"

using namespace std;

namespace Astroid {
  Action::Action (refptr<NotmuchThread> nmt) {

    threads.push_back (nmt);

  }

  Action::Action (vector<refptr<NotmuchThread>> nmts) :
    threads (nmts) { }

  bool Action::undoable () {
    return false;
  }

  void Action::emit (Db * db) {
    for (auto &t : threads) {
      astroid->global_actions->emit_thread_updated (db, t->thread_id);
    }
  }
}

