# include <iostream>

# include <gtkmm.h>

# include "gulp.hh"


using namespace std;

/* globally available static instance of the Gulp */
Gulp::Gulp * (Gulp::gulp);

int main (int argc, char **argv) {
  Gulp::gulp = new Gulp::Gulp ();
  return Gulp::gulp->main (argc, argv);
}

namespace Gulp {

  Gulp::Gulp () {
  }

  int Gulp::main (int argc, char **argv) {
    cout << "gulp - v" << GIT_DESC << endl;
    return 0;
  }

}

