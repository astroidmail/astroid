# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchThread>, ustring);
  };

  class SpamAction : public TagAction {
    public:
      const ustring spam  = "spam";
      const ustring inbox = "inbox";
      SpamAction (refptr<NotmuchThread>);
  };

  class MuteAction : public TagAction {
    public:
      const ustring muted = "muted";
      MuteAction (refptr<NotmuchThread>);
  };

}

