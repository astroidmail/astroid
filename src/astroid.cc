# include <iostream>
# include <vector>
# include <memory>

# include <gtkmm.h>
# include <gtkmm/window.h>

/* program options */
# include <boost/program_options.hpp>

# include "astroid.hh"
# include "version.hh"
# include "db.hh"
# include "config.hh"
# include "account_manager.hh"
# include "contacts.hh"

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

    namespace po = boost::program_options;
    po::options_description desc ("options");
    desc.add_options ()
      ( "help,h", "print this help message")
      ( "config,c", po::value<ustring>(), "config file, default: $XDG_CONFIG_HOME/astroid/config");

    po::variables_map vm;
    po::store ( po::command_line_parser (argc, argv).options(desc).run(), vm );

    if (vm.count ("help")) {
      cout << desc << endl;

      exit (1);
    }

    /* load config */
    if (vm.count("config")) {
      config = new Config (vm["config"].as<ustring>().c_str());
    } else {
      config = new Config ();
    }

    /* set up gtk */
    int aargc = 1;
    app = Gtk::Application::create (aargc, argv, "org.astroid");

    /* notmuch db */
    db = new Db ();

    /* gmime settings */
    g_mime_init (0); // utf-8 is default

    /* set up accounts */
    accounts = new AccountManager ();

    /* set up contacts */
    contacts = new Contacts ();

    /* start up default window with default buffers */
    MainWindow * mw = new MainWindow (); // is freed / destroyed by application
    mw->add_mode (new ThreadIndex (mw, "tag:inbox"));

    main_windows.push_back (mw);
    main_windows.shrink_to_fit ();

    app->run (*mw);

    /* clean up and exit */
    main_windows.clear ();
    delete accounts;
    delete contacts;
    delete db;
    delete config;

    return 0;
  }

  void Astroid::quit () {
    cout << "astroid: goodbye!" << endl;

    /* remove modes */
    for (auto mw : main_windows) {
      mw->remove_all_modes ();
    }

    app->quit ();
  }

}

