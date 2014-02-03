# pragma once

# include <vector>

# include "proto.hh"

using namespace std;

namespace Gulp {
  class Gulp {
    public:
      Gulp ();
      int main (int, char**);

      Db *db;




  };

  /* globally available instance of our main Gulp-class */
  extern Gulp * gulp;
}

