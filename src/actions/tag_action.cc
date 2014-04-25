# include <iostream>
# include <vector>

# include "action.hh"
# include "db.hh"

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

  bool TagAction::undoable () {
    return true;
  }

  bool TagAction::doit () {
    cout << "tag_action: " << thread->thread_id << ", add: ";
    for_each (add.begin(),
              add.end(),
              [&](ustring t) {
                cout << t << ", ";
              });

    cout << "remove: ";
    for_each (remove.begin(),
              remove.end(),
              [&](ustring t) {
                cout << t << ", ";
              });

    cout << endl;

    bool res = true;

    for_each (add.begin(),
              add.end(),
              [&](ustring t) {
                res &= thread->add_tag (t);
              });

    for_each (remove.begin(),
              remove.end(),
              [&](ustring t) {
                res &= thread->remove_tag (t);
              });

    return res;
  }

  bool TagAction::undo () {
    cout << "tag_action: undo." << endl;

    swap (add, remove);
    return doit ();
  }

}
