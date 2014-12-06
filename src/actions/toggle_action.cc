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
    : TagAction (nmt) {
      if (find (nmt->tags.begin(), nmt->tags.end(), spam) != nmt->tags.end())
      {
        remove.push_back (spam);
      } else {
        add.push_back (spam);
        if (find (nmt->tags.begin(), nmt->tags.end(), inbox) != nmt->tags.end())
        {
          remove.push_back (inbox);
        }
      }
    }

  MuteAction::MuteAction (refptr<NotmuchThread> nmt)
    : TagAction (nmt) {
      if (find (nmt->tags.begin(), nmt->tags.end(), muted) != nmt->tags.end())
      {
        remove.push_back (muted);
      } else {
        add.push_back (muted);
      }
    }
}
