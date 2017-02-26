# include <iostream>
# include <vector>

# include "action.hh"
# include "db.hh"

# include "tag_action.hh"


using namespace std;

namespace Astroid {
  TagAction::TagAction ( refptr<NotmuchItem> nmt) {
    taggables.push_back (nmt);
  }

  TagAction::TagAction (
      refptr<NotmuchItem> nmt,
      vector<ustring> _add,
      vector<ustring> _remove)
  : add (_add), remove (_remove)
  {
    taggables.push_back (nmt);
  }

  TagAction::TagAction ( vector<refptr<NotmuchItem>> nmts)
  : taggables (nmts)
  {
  }

  TagAction::TagAction (
      vector<refptr<NotmuchItem>> nmts,
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
      LOG (info) << "tag_action: " << tagged->str ();


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
    LOG (info) << "tag_action: undo.";

    swap (add, remove);
    return doit (db);
  }

  void TagAction::emit (Db * db) {
    for (auto &t : taggables) {
      t->emit_updated (db);
    }
  }

}
