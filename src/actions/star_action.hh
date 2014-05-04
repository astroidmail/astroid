# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class StarAction : public TagAction {
    public:
      StarAction (refptr<NotmuchThread>);
  };

}

