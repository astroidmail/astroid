# pragma once

# include <vector>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class ActionManager {
    public:
      ActionManager ();

      MainWindow * main_window;

      vector<refptr<Action>> actions;

      bool doit (refptr<Action>);
      bool undo ();

  };
}
