# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class DiffTagAction : public TagAction {
    public:
      DiffTagAction (std::vector<refptr<NotmuchThread>>,
                     std::vector<ustring> add,
                     std::vector<ustring> rem);

      static DiffTagAction * create (std::vector<refptr<NotmuchThread>>, ustring);

      virtual bool doit (Db *) override;
      virtual bool undo (Db *) override;

    private:
      struct ThreadAction {
        refptr<NotmuchThread> thread;
        std::vector<ustring>  add;
        std::vector<ustring>  remove;
      };

      std::vector<ThreadAction> thread_actions;
  };
}

