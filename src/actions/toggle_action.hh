# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchThread>, ustring);
  };

  class SpamAction : public ToggleAction {
    public:
      SpamAction (refptr<NotmuchThread>);
  };

  class MuteAction : public ToggleAction {
    public:
      MuteAction (refptr<NotmuchThread>);
  };

}

