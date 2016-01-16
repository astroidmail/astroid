# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchThread>, ustring);
      ToggleAction (std::vector<refptr<NotmuchThread>>, ustring);
      ustring toggle_tag;

      /* for toggleaction undo == doit, which works with
       * how it is defined in tagaction. */
      virtual bool doit (Db *) override;
  };

  class SpamAction : public ToggleAction {
    public:
      SpamAction (refptr<NotmuchThread>);
      SpamAction (std::vector<refptr<NotmuchThread>>);
  };

  class MuteAction : public ToggleAction {
    public:
      MuteAction (refptr<NotmuchThread>);
      MuteAction (std::vector<refptr<NotmuchThread>>);
  };

}

