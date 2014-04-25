# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class TagAction : public Action {
    public:
      TagAction (refptr<NotmuchThread>);

      TagAction (
          refptr<NotmuchThread>,
          vector<ustring>,
          vector<ustring>);

      vector<ustring> add;
      vector<ustring> remove;

      bool doit () override;
      bool undo () override;
      bool undoable () override;

  };

}
