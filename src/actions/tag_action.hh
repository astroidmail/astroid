# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class TagAction : public Action {
    public:
      TagAction (refptr<NotmuchTaggable>);

      TagAction (
          refptr<NotmuchTaggable>,
          const std::vector<ustring>,
          const std::vector<ustring>);

      TagAction (std::vector<refptr<NotmuchTaggable>>);

      TagAction (
          std::vector<refptr<NotmuchTaggable>>,
          const std::vector<ustring>,
          const std::vector<ustring>);

      std::vector<refptr<NotmuchTaggable>> taggables;
      std::vector<ustring> add;
      std::vector<ustring> remove;

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;
      virtual bool undoable () override;
      virtual void emit (Db *) override;

  };

}
