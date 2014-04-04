# include <iostream>
# include <vector>
# include <memory>

# include <gtkmm.h>
# include <gtkmm/window.h>

# include "astroid.hh"
# include "db.hh"

/* UI */
# include "main_window.hh"
# include "modes/thread_index.hh"

/* gmime */
# include <gmime/gmime.h>

using namespace std;

/* globally available static instance of the Astroid */
Astroid::Astroid * (Astroid::astroid);

int main (int argc, char **argv) {
  Astroid::astroid = new Astroid::Astroid ();
  return Astroid::astroid->main (argc, argv);
}

namespace Astroid {

  Astroid::Astroid () {
  }

  int Astroid::main (int argc, char **argv) {
    cout << "welcome to astroid! - " << GIT_DESC << endl;

    /* set up gtk */
    app = Gtk::Application::create (argc, argv, "org.astroid");

    /* notmuch db */
    db = new Db ();

    /* gmime settings */
    g_mime_init (0); // utf-8 is default

    /* start up default window with default buffers */
    MainWindow mw;
    mw.add_mode (new ThreadIndex (&mw, "tag:inbox"));
    mw.add_mode (new ThreadIndex (&mw, "tag:inbox and tag:unread"));
    mw.add_mode (new ThreadIndex (&mw, "astrid"));

    main_windows.push_back (&mw);
    main_windows.shrink_to_fit ();

    app->run (mw);

    /* clean up and exit */
    main_windows.clear ();
    delete db;


    return 0;
  }

  void Astroid::quit () {
    cout << "goodbye!" << endl;
    app->quit ();

  }

}

