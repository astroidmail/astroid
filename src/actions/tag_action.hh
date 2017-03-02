# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class TagAction : public Action {
    public:
      TagAction (refptr<NotmuchItem>);

      TagAction (
          refptr<NotmuchItem>,
          const std::vector<ustring>,
          const std::vector<ustring>);

      TagAction (std::vector<refptr<NotmuchItem>>);

      TagAction (
          std::vector<refptr<NotmuchItem>>,
          const std::vector<ustring>,
          const std::vector<ustring>);

      std::vector<refptr<NotmuchItem>> taggables;
      std::vector<ustring> add;
      std::vector<ustring> remove;

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;
      virtual void emit (Db *) override;

  };

}
