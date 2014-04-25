# include <iostream>
# include <vector>
# include <algorithm>

# include "action.hh"
# include "db.hh"

# include "tag_action.hh"
# include "archive_action.hh"


using namespace std;

namespace Astroid {
  ArchiveAction::ArchiveAction (refptr<NotmuchThread> nmt)
  : TagAction(nmt)
  {
    if (find (nmt->tags.begin(), nmt->tags.end(), "inbox") != nmt->tags.end ()) {
      remove.push_back ("inbox");
    } else {
      add.push_back ("inbox");
    }
  }
}
