# include <iostream>

# include "action.hh"
# include "db.hh"

# include "tag_action.hh"


using namespace std;

namespace Astroid {
  TagAction::TagAction (refptr<NotmuchThread> nmt)
  : Action(nmt)
  {

  }

  bool TagAction::doit () { }
  bool TagAction::undo () { }

}
