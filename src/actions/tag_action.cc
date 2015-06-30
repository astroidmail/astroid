# include <iostream>
# include <vector>

# include "action.hh"
# include "db.hh"
# include "log.hh"

# include "tag_action.hh"


using namespace std;

namespace Astroid {
  TagAction::TagAction ( refptr<NotmuchThread> nmt)
  : Action(nmt)
  {
  }

  TagAction::TagAction (
      refptr<NotmuchThread> nmt,
      vector<ustring> _add,
      vector<ustring> _remove)
  : Action(nmt), add (_add), remove (_remove)
  {
  }

  TagAction::TagAction ( vector<refptr<NotmuchThread>> nmts)
  : Action(nmts)
  {
  }

  TagAction::TagAction (
      vector<refptr<NotmuchThread>> nmts,
      vector<ustring> _add,
      vector<ustring> _remove)
  : Action(nmts), add (_add), remove (_remove)
  {
  }

  bool TagAction::undoable () {
    return true;
  }

  bool TagAction::doit (Db * db) {
    bool res = true;
    for (auto &thread : threads) {
      log << info << "tag_action: " << thread->thread_id << ", add: ";
      for_each (add.begin(),
                add.end(),
                [&](ustring t) {
                  log << t << ", ";
                });

      log << "remove: ";
      for_each (remove.begin(),
                remove.end(),
                [&](ustring t) {
                  log << t << ", ";
                });

      log << endl;


      for_each (add.begin(),
                add.end(),
                [&](ustring t) {
                  res &= thread->add_tag (db, t);
                });

      for_each (remove.begin(),
                remove.end(),
                [&](ustring t) {
                  res &= thread->remove_tag (db, t);
                });

    }
    return res;
  }

  bool TagAction::undo (Db * db) {
    log << info << "tag_action: undo." << endl;

    swap (add, remove);
    return doit (db);
  }

}
