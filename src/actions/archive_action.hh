# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ArchiveAction : public TagAction {
    public:
      ArchiveAction (refptr<NotmuchThread>);
  };

}

