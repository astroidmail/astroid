# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class TagAction : public Action {
    public:
      TagAction (refptr<NotmuchThread>);

      TagAction (
          refptr<NotmuchThread>,
          const std::vector<ustring>,
          const std::vector<ustring>);

      TagAction (std::vector<refptr<NotmuchThread>>);

      TagAction (
          std::vector<refptr<NotmuchThread>>,
          const std::vector<ustring>,
          const std::vector<ustring>);

      std::vector<ustring> add;
      std::vector<ustring> remove;

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;

  };

}
