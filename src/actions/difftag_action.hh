# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"
# include "tag_action.hh"

namespace Astroid {
  class DiffTagAction : public TagAction {
    public:
      DiffTagAction (std::vector<refptr<NotmuchTaggable>>,
                     std::vector<ustring> add,
                     std::vector<ustring> rem);

      static DiffTagAction * create (std::vector<refptr<NotmuchTaggable>>, ustring);

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;

    private:
      struct TaggableAction {
        refptr<NotmuchTaggable> taggable;
        std::vector<ustring>    add;
        std::vector<ustring>    remove;
      };

      std::vector<TaggableAction> taggable_actions;
  };
}

