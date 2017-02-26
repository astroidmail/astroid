# include <iostream>
# include <vector>
# include <algorithm>

# include "action.hh"
# include "db.hh"

# include "tag_action.hh"
# include "toggle_action.hh"


using namespace std;

namespace Astroid {
  ToggleAction::ToggleAction (refptr<NotmuchItem> nmt, ustring _toggle_tag)
  : TagAction(nmt)
  {
    toggle_tag = _toggle_tag;
  }

  ToggleAction::ToggleAction (vector<refptr<NotmuchItem>> nmts, ustring _toggle_tag)
  : TagAction(nmts)
  {
    toggle_tag = _toggle_tag;
  }

  bool ToggleAction::doit (Db * db) {
    bool res = true;

    for (auto &tagged : taggables) {
      LOG (debug) << "toggle_action: " << tagged->str ();

      if (find (tagged->tags.begin(), tagged->tags.end(), toggle_tag) != tagged->tags.end ()) {
        remove.push_back (toggle_tag);
      } else {
        add.push_back (toggle_tag);
      }


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

  SpamAction::SpamAction (refptr<NotmuchItem> nmt)
    : ToggleAction (nmt, "spam") {
    }

  SpamAction::SpamAction (vector<refptr<NotmuchItem>> nmts)
    : ToggleAction (nmts, "spam") {
    }

  MuteAction::MuteAction (refptr<NotmuchItem> nmt)
    : ToggleAction (nmt, "muted") {
    }

  MuteAction::MuteAction (vector<refptr<NotmuchItem>> nmts)
    : ToggleAction (nmts, "muted") {
    }
}
