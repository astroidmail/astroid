# include <iostream>
# include <vector>
# include <memory>

# include <gtkmm.h>
# include <gtkmm/window.h>

# include "gulp.hh"
# include "db.hh"

/* UI */
# include "main_window.hh"
# include "modes/thread_index.hh"

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
    cout << "welcome to gulp! - v" << GIT_DESC << endl;

    /* set up gtk */
    app = Gtk::Application::create (argc, argv, "org.gulp");

    db = new Db ();

    /* start up default window with default buffers */
    MainWindow mw;
    mw.add_mode (new ThreadIndex ("label:inbox"));
    mw.add_mode (new ThreadIndex ("astrid"));

    main_windows.push_back (&mw);
    main_windows.shrink_to_fit ();

    app->run (mw);

    /* clean up and exit */
    main_windows.clear ();
    delete db;


    return 0;
  }

  void Gulp::quit () {
    cout << "goodbye!" << endl;
    app->quit ();

  }

}

