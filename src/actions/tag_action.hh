# pragma once

using namespace std;

# include "proto.hh"
# include "action.hh"

namespace Astroid {
  class TagAction : public Action {
    public:
      TagAction (refptr<NotmuchThread>);

      bool doit () override;
      bool undo () override;

  };

}
