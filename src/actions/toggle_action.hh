# pragma once

# include <vector>

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class ToggleAction : public TagAction {
    public:
      ToggleAction (refptr<NotmuchTaggable>, ustring);
      ToggleAction (std::vector<refptr<NotmuchTaggable>>, ustring);
      ustring toggle_tag;

      /* for toggleaction undo == doit, which works with
       * how it is defined in tagaction. */
      virtual bool doit (Db *) override;
  };

  class SpamAction : public ToggleAction {
    public:
      SpamAction (refptr<NotmuchTaggable>);
      SpamAction (std::vector<refptr<NotmuchTaggable>>);
  };

  class MuteAction : public ToggleAction {
    public:
      MuteAction (refptr<NotmuchTaggable>);
      MuteAction (std::vector<refptr<NotmuchTaggable>>);
  };

}

