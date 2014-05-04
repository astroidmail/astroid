# include <iostream>
# include <vector>
# include <algorithm>

# include "action.hh"
# include "db.hh"

# include "tag_action.hh"
# include "star_action.hh"


using namespace std;

namespace Astroid {
  StarAction::StarAction (refptr<NotmuchThread> nmt)
  : TagAction(nmt)
  {
    if (find (nmt->tags.begin(), nmt->tags.end(), "starred") != nmt->tags.end ()) {
      remove.push_back ("starred");
    } else {
      add.push_back ("starred");
    }
  }
}
