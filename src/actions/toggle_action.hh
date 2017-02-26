# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchItem>, ustring);
      ToggleAction (std::vector<refptr<NotmuchItem>>, ustring);
      ustring toggle_tag;

      /* for toggleaction undo == doit, which works with
       * how it is defined in tagaction. */
      virtual bool doit (Db *) override;
  };

  class SpamAction : public ToggleAction {
    public:
      SpamAction (refptr<NotmuchItem>);
      SpamAction (std::vector<refptr<NotmuchItem>>);
  };

  class MuteAction : public ToggleAction {
    public:
      MuteAction (refptr<NotmuchItem>);
      MuteAction (std::vector<refptr<NotmuchItem>>);
  };

}

