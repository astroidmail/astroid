# pragma once

# include <vector>


using namespace std;

namespace Gulp {
  class Gulp {
    public:
      Gulp ();
      int main (int, char**);




  };

  /* globally available instance of our main Gulp-class */
  extern Gulp * gulp;
}

