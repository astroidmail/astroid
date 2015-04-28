# include <iostream>
# include <vector>
# include <algorithm>

# include "action.hh"
# include "db.hh"

# include "tag_action.hh"
# include "toggle_action.hh"


using namespace std;

namespace Astroid {
  ToggleAction::ToggleAction (refptr<NotmuchThread> nmt, ustring toggle_tag)
  : TagAction(nmt)
  {
    if (find (nmt->tags.begin(), nmt->tags.end(), toggle_tag) != nmt->tags.end ()) {
      remove.push_back (toggle_tag);
    } else {
      add.push_back (toggle_tag);
    }
  }

  SpamAction::SpamAction (refptr<NotmuchThread> nmt)
    : ToggleAction (nmt, "spam") {
    }

  MuteAction::MuteAction (refptr<NotmuchThread> nmt)
    : ToggleAction (nmt, "muted") {
    }
}
