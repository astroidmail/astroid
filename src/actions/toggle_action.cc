# include <iostream>
# include <vector>
# include <algorithm>

# include "log.hh"
# include "action.hh"
# include "db.hh"

# include "tag_action.hh"
# include "toggle_action.hh"


using namespace std;

namespace Astroid {
  ToggleAction::ToggleAction (refptr<NotmuchTaggable> nmt, ustring _toggle_tag)
  : TagAction(nmt)
  {
    toggle_tag = _toggle_tag;
  }

  ToggleAction::ToggleAction (vector<refptr<NotmuchTaggable>> nmts, ustring _toggle_tag)
  : TagAction(nmts)
  {
    toggle_tag = _toggle_tag;
  }

  bool ToggleAction::doit (Db * db) {
    bool res = true;

    for (auto &tagged : taggables) {
      log << debug << "toggle_action: " << tagged->str () << ", add: ";

      if (find (tagged->tags.begin(), tagged->tags.end(), toggle_tag) != tagged->tags.end ()) {
        remove.push_back (toggle_tag);
      } else {
        add.push_back (toggle_tag);
      }

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


      remove.clear ();
      add.clear ();
    }

    return res;
  }

  SpamAction::SpamAction (refptr<NotmuchTaggable> nmt)
    : ToggleAction (nmt, "spam") {
    }

  SpamAction::SpamAction (vector<refptr<NotmuchTaggable>> nmts)
    : ToggleAction (nmts, "spam") {
    }

  MuteAction::MuteAction (refptr<NotmuchTaggable> nmt)
    : ToggleAction (nmt, "muted") {
    }

  MuteAction::MuteAction (vector<refptr<NotmuchTaggable>> nmts)
    : ToggleAction (nmts, "muted") {
    }
}
