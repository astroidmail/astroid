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
          const vector<ustring>,
          const vector<ustring>);

      TagAction (vector<refptr<NotmuchThread>>);

      TagAction (
          vector<refptr<NotmuchThread>>,
          const vector<ustring>,
          const vector<ustring>);

      vector<ustring> add;
      vector<ustring> remove;

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;

  };

}
