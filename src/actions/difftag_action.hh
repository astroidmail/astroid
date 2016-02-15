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

    public:
      static DiffTagAction * create (std::vector<refptr<NotmuchThread>>, ustring);
  };
}

