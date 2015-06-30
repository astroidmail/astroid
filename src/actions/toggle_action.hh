# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchThread>, ustring);
      ToggleAction (vector<refptr<NotmuchThread>>, ustring);
      ustring toggle_tag;

      /* for toggleaction undo == doit, which works with
       * how it is defined in tagaction. */
      virtual bool doit (Db *) override;
  };

  class SpamAction : public ToggleAction {
    public:
      SpamAction (refptr<NotmuchThread>);
      SpamAction (vector<refptr<NotmuchThread>>);
  };

  class MuteAction : public ToggleAction {
    public:
      MuteAction (refptr<NotmuchThread>);
      MuteAction (vector<refptr<NotmuchThread>>);
  };

}

