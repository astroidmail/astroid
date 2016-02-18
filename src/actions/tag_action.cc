# include <iostream>
# include <vector>

# include "action.hh"
# include "db.hh"
# include "log.hh"

# include "tag_action.hh"


using namespace std;

namespace Astroid {
  TagAction::TagAction ( refptr<NotmuchTaggable> nmt) {
    taggables.push_back (nmt);
  }

  TagAction::TagAction (
      refptr<NotmuchTaggable> nmt,
      vector<ustring> _add,
      vector<ustring> _remove)
  : add (_add), remove (_remove)
  {
    taggables.push_back (nmt);
  }

  TagAction::TagAction ( vector<refptr<NotmuchTaggable>> nmts)
  : taggables (nmts)
  {
  }

  TagAction::TagAction (
      vector<refptr<NotmuchTaggable>> nmts,
      vector<ustring> _add,
      vector<ustring> _remove)
  : taggables(nmts), add (_add), remove (_remove)
  {
  }

  bool TagAction::undoable () {
    return true;
  }

  bool TagAction::doit (Db * db) {
    bool res = true;
    for (auto &tagged : taggables) {
      log << info << "tag_action: " << tagged->str () << ", add: ";
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
                  res &= tagged->add_tag (db, t);
                });

      for_each (remove.begin(),
                remove.end(),
                [&](ustring t) {
                  res &= tagged->remove_tag (db, t);
                });

    }
    return res;
  }

  bool TagAction::undo (Db * db) {
    log << info << "tag_action: undo." << endl;

    swap (add, remove);
    return doit (db);
  }

  void TagAction::emit (Db * db) {
    for (auto &t : taggables) {
      t->emit_updated (db);
    }
  }

}
