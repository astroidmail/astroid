# include <iostream>
# include <vector>
# include <memory>

# include <gtkmm.h>
# include <gtkmm/window.h>

/* program options */
# include <boost/program_options.hpp>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "build_config.hh"
# include "db.hh"
# include "config.hh"
# include "account_manager.hh"
# include "actions/action_manager.hh"
# include "utils/date_utils.hh"
# include "log.hh"
# include "poll.hh"

/* UI */
# include "main_window.hh"
# include "modes/thread_index.hh"
# include "modes/edit_message.hh"

/* gmime */
# include <gmime/gmime.h>

using namespace std;
using namespace boost::filesystem;

/* globally available static instance of the Astroid */
Astroid::Astroid * (Astroid::astroid);
Astroid::Log Astroid::log;

namespace Astroid {
  Astroid::Astroid () {
    setlocale (LC_ALL, "");
    Glib::init ();

    log.add_out_stream (&cout);

    log << info << "welcome to astroid! - " << GIT_DESC << endl;

    string charset;
    Glib::get_charset(charset);
    log << info << "utf8: " << Glib::get_charset () << ", " << charset << endl;

    /* user agent */
    user_agent = ustring::compose ("astroid/v%1 (https://github.com/gauteh/astroid)", GIT_DESC);

    /* gmime settings */
    g_mime_init (0); // utf-8 is default
  }

  int Astroid::main (int argc, char **argv) {
    /* options */
    namespace po = boost::program_options;
    po::options_description desc ("options");
    desc.add_options ()
      ( "help,h", "print this help message")
      ( "config,c", po::value<ustring>(), "config file, default: $XDG_CONFIG_HOME/astroid/config")
      ( "new-config,n", "make new default config, then exit")
      ( "mailto,m", po::value<ustring>(), "compose mail with mailto url");

    po::variables_map vm;
    po::store ( po::command_line_parser (argc, argv).options(desc).run(), vm );

    if (vm.count ("help")) {
      cout << desc << endl;

      exit (0);
    }

    /* make new config {{{ */
    if (vm.count("new-config")) {
      log << info << "creating new config.." << endl;
      ustring cnf;

      Config ncnf (false, true);

      if (vm.count("config")) {
        cnf = vm["config"].as<ustring>().c_str();

        if (exists (path(cnf))) {
          log << error << "the config file: " << cnf << " already exists." << endl;
          exit (1);
        }

        ncnf.config_file = path(cnf);

      } else {
        /* use default */
        if (exists(ncnf.config_file)) {
          log << error << "the config file: " << ncnf.config_file.c_str() << " already exists." << endl;
          exit (1);
        }
      }

      log << info << "writing default config to: " << ncnf.config_file.c_str() << endl;
      ncnf.load_config (true);

      exit (0);
    } // }}}

    bool domailto = false;
    ustring mailtourl;

    if (vm.count("mailto")) {
      domailto = true;
      mailtourl = vm["mailto"].as<ustring>();

      log << debug << "astroid: composing mail to: " << mailtourl << endl;
    }

    /* set up gtk */
    log << debug << "loading gtk.." << endl;
    int aargc = 1; // TODO: allow GTK to get some options aswell.
    app = Gtk::Application::create (aargc, argv, "org.astroid");

    app->register_application ();

    if (app->is_remote ()) {
      log << warn << "astroid: instance already running, opening new window.." << endl;

      if (domailto) {
        Glib::Variant<ustring> param = Glib::Variant<ustring>::create (mailtourl);
        app->activate_action ("mailto",
            param
            );

      } else {
        app->activate ();

      }

      return 0;
    } else {
      /* we are the main instance */
      app->signal_activate().connect (sigc::mem_fun (this,
            &Astroid::on_signal_activate));

      mailto = Gio::SimpleAction::create ("mailto", Glib::VariantType ("s"));

      app->add_action (mailto);

      mailto->set_enabled (true);
      mailto->signal_activate ().connect (
          sigc::mem_fun (this, &Astroid::on_mailto_activate));
    }

    /* load config */
    if (vm.count("config")) {
      config = new Config (vm["config"].as<ustring>().c_str());
    } else {
      config = new Config ();
    }

    /* set up static classes */
    Date::init ();

    /* set up accounts */
    accounts = new AccountManager ();

    /* set up contacts */
    //contacts = new Contacts ();

    /* set up global actions */
    global_actions = new GlobalActions ();

    /* set up poller */
    poll = new Poll ();

    if (domailto) {
      MainWindow * mw = open_new_window (false);
      send_mailto (mw, mailtourl);
    } else {
      open_new_window ();
    }
    app->run ();

    return 0;
  }

  void Astroid::main_test () {
    config = new Config (true);

    /* set up static classes */
    Date::init ();

    /* set up accounts */
    accounts = new AccountManager ();

    /* set up contacts */
    //contacts = new Contacts ();

    /* set up global actions */
    global_actions = new GlobalActions ();

    /* set up poller */
    poll = new Poll ();
  }

  Astroid::~Astroid () {
    /* clean up and exit */
    if (accounts != NULL) delete accounts;
    //if (contacts != NULL) delete contacts;
    if (config != NULL) delete config;
    if (poll != NULL) delete poll;
    if (global_actions != NULL) delete global_actions;

    log << info << "astroid: goodbye!" << endl;
    log.del_out_stream (&cout);
  }

  MainWindow * Astroid::open_new_window (bool open_defaults) {
    log << warn << "astroid: starting a new window.." << endl;

    /* set up a new main window */

    /* start up default window with default buffers */
    MainWindow * mw = new MainWindow (); // is freed / destroyed by application

    if (open_defaults) {
      ptree qpt = config->config.get_child ("startup.queries");

      for (const auto &kv : qpt) {
        ustring name = kv.first;
        ustring query = kv.second.data();

        log << info << "astroid: got query: " << name << ": " << query << endl;
        mw->add_mode (new ThreadIndex (mw, query));
      }

      mw->set_active (0);
    }

    app->add_window (*mw);
    mw->show_all ();

    return mw;
  }

  void Astroid::on_signal_activate () {
    if (activated) {
      open_new_window ();
    } else {
      /* we'll get one activated signal from ourselves first */
      activated = true;
    }
  }

  void Astroid::on_mailto_activate (const Glib::VariantBase & parameter) {
    Glib::Variant<ustring> url = Glib::VariantBase::cast_dynamic<Glib::Variant<ustring>> (parameter);

    send_mailto (open_new_window (false), url.get());
  }

  void Astroid::send_mailto (MainWindow * mw, ustring url) {
    log << info << "astorid: mailto: " << url << endl;

    ustring scheme = Glib::uri_parse_scheme (url);
    if (scheme.length () > 0) {
      /* we got an mailto url */
      url = url.substr(scheme.length(), url.length () - scheme.length());

      ustring to, cc, bcc, subject, body;

      ustring::size_type pos = url.find ("?");
      ustring::size_type next;
      if (pos == ustring::npos) pos = url.length ();
      to = url.substr (0, pos);

      /* TODO: need to finish the rest of the fields */
      mw->add_mode (new editmessage (mw, to));

    } else {
      /* we probably just got the address on the cmd line */
      mw->add_mode (new editmessage (mw, url));
    }
  }

}

