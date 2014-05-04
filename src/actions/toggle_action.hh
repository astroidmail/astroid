# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchThread>, ustring);
  };

}

