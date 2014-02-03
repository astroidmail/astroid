# pragma once

# include <vector>
# include <memory>

# include "proto.hh"

using namespace std;

namespace Gulp {
  class Gulp {
    public:
      Gulp ();
      int main (int, char**);

      Db *db;

      /* list of main windows */
      vector<MainWindow*>  main_windows;


  };

  /* globally available instance of our main Gulp-class */
  extern Gulp * gulp;
}

