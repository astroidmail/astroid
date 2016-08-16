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
# include "actions/action.hh"
# include "utils/date_utils.hh"
# include "utils/utils.hh"

# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

# include "poll.hh"

/* UI */
# include "main_window.hh"
# include "modes/thread_index/thread_index.hh"
# include "modes/edit_message.hh"
# include "modes/saved_searches.hh"

/* gmime */
# include <gmime/gmime.h>

using namespace std;
using namespace boost::filesystem;

/* globally available static instance of the Astroid */
Astroid::Astroid * (Astroid::astroid);
const char* const Astroid::Astroid::version = GIT_DESC;

namespace Astroid {
  Astroid::Astroid () {
    setlocale (LC_ALL, "");
    Glib::init ();

    LOG (info) << "welcome to astroid! - " << Astroid::version;
    LOG (info) << "welcome to astroid! - " << Astroid::version;

    string charset;
    Glib::get_charset(charset);
    LOG (debug) << "utf8: " << Glib::get_charset () << ", " << charset;

    if (!Glib::get_charset()) {
      LOG (error) << "astroid needs an UTF-8 locale! this is probably not going to work.";
    }

    /* user agent */
    user_agent = ustring::compose ("astroid/%1 (https://github.com/astroidmail/astroid)", Astroid::version);

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
      ( "test-config,t", "use test config (same as used when tests are run), only makes sense from the source root")
      ( "mailto,m", po::value<ustring>(), "compose mail with mailto url or address")
      ( "no-auto-poll", "do not poll automatically")
      ( "log,l", po::value<ustring>(), "log to file")
      ( "append-log,a", "append to log file")
# ifndef DISABLE_PLUGINS
      ( "disable-plugins", "disable plugins");
# else
      ;
# endif

    po::variables_map vm;

    bool show_help = false;

    try {
      po::store ( po::parse_command_line (argc, argv, desc), vm );
    } catch (po::unknown_option &ex) {
      cout << "unknown option" << endl;
      cout << ex.what() << endl;
      show_help = true;

    }

    show_help |= vm.count("help");
    bool test_config = vm.count("test-config");

    if (show_help) {

      cout << desc << endl;
      exit (0);

    }

    /*
    if (vm.count ("log")) {
      ustring lfile = vm["log"].as<ustring> ();
      if (vm.count ("append-log")) {
        logf.open (lfile, std::ofstream::out | std::ofstream::app);
      } else {
        logf.open (lfile, std::ofstream::out);
      }
      if (logf.good ()) {
        log.add_out_stream (&logf);
        LOG (info) << "logging to: " << lfile;
      } else {
        if (logf.is_open ()) logf.close ();
        LOG (error) << "could not open: " << lfile << " for logging..";
      }
    }
    */

    /* make new config {{{ */
    if (vm.count("new-config")) {
      if (test_config) {
        LOG (error) << "--new-config cannot be specified together with --test-config.";
        exit (1);
      }

      LOG (info) << "creating new config..";
      ustring cnf;

      Config ncnf (false, true);

      if (vm.count("config")) {
        cnf = vm["config"].as<ustring>().c_str();

        if (exists (path(cnf))) {
          LOG (error) << "the config file: " << cnf << " already exists.";
          exit (1);
        }

        ncnf.std_paths.config_file = path(cnf);

      } else {
        /* use default */
        if (exists(ncnf.std_paths.config_file)) {
          LOG (error) << "the config file: " << ncnf.std_paths.config_file.c_str() << " already exists.";
          exit (1);
        }
      }

      LOG (info) << "writing default config to: " << ncnf.std_paths.config_file.c_str();
      ncnf.load_config (true);

      exit (0);
    } // }}}

    bool no_auto_poll = false;

    if (vm.count ("no-auto-poll")) {
      LOG (info) << "astroid: automatic polling is off.";

      no_auto_poll = true;
    }

# ifndef DISABLE_PLUGINS
    bool disable_plugins = vm.count ("disable-plugins");
# endif

    bool domailto = false;
    ustring mailtourl;

    if (vm.count("mailto")) {
      domailto = true;
      mailtourl = vm["mailto"].as<ustring>();

      LOG (debug) << "astroid: composing mail to: " << mailtourl;
    }

    /* set up gtk */
    LOG (debug) << "loading gtk..";
    int aargc = 1; // TODO: allow GTK to get some options aswell.
    app = Gtk::Application::create (aargc, argv, "org.astroid");

    app->register_application ();

    if (app->is_remote ()) {
      LOG (warn) << "astroid: instance already running, opening new window..";

      if (no_auto_poll) {
        LOG (warn) << "astroid: specifying no-auto-poll only makes sense when starting a new astroid instance, ignoring.";
      }

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
      if (test_config) {
        LOG (error) << "--config cannot be specified together with --test-config.";
        exit (1);
      }

      LOG (info) << "astroid: loading config: " << vm["config"].as<ustring>().c_str();
      m_config = new Config (vm["config"].as<ustring>().c_str());
    } else {
      if (test_config) {
        m_config = new Config (true);
      } else {
        m_config = new Config ();
      }
    }

    /* output db location */
    ustring db_path = ustring (notmuch_config().get<string> ("database.path"));
    LOG (info) << "notmuch db: " << db_path;

    /* set up static classes */
    Date::init ();
    Utils::init ();
    Db::init ();
    Keybindings::init ();
    SavedSearches::init ();

    /* set up accounts */
    accounts = new AccountManager ();

# ifndef DISABLE_PLUGINS
    /* set up plugins */
    plugin_manager = new PluginManager (disable_plugins, in_test ());
    plugin_manager->astroid_extension = new PluginManager::AstroidExtension (this);
# endif

    /* set up contacts */
    //contacts = new Contacts ();

    /* set up global actions */
    actions = new ActionManager ();

    /* set up poller */
    poll = new Poll (!no_auto_poll);

    if (domailto) {
      MainWindow * mw = open_new_window (false);
      send_mailto (mw, mailtourl);
    } else {
      open_new_window ();
    }
    app->run ();

    on_quit ();

    return 0;
  }

  const boost::property_tree::ptree& Astroid::config (const std::string& id) const {
    return m_config->config.get_child(id);
  }

  const boost::property_tree::ptree& Astroid::notmuch_config () const {
    return m_config->notmuch_config;
  }

  const StandardPaths& Astroid::standard_paths() const {
    return m_config->std_paths;
  }

  void Astroid::main_test () {
    m_config = new Config (true);

    /* set up static classes */
    Date::init ();
    Utils::init ();
    Db::init ();
    SavedSearches::init ();

    /* set up accounts */
    accounts = new AccountManager ();

# ifndef DISABLE_PLUGINS
    /* set up plugins */
    plugin_manager = new PluginManager (false, true);
    plugin_manager->astroid_extension = new PluginManager::AstroidExtension (this);
# endif

    /* set up contacts */
    //contacts = new Contacts ();

    /* set up global actions */
    actions = new ActionManager ();

    /* set up poller */
    poll = new Poll (false);
  }

  bool Astroid::in_test () {
    return m_config->test;
  }

  void Astroid::on_quit () {
    LOG (debug) << "astroid: quitting..";

    /* clean up and exit */
    if (actions) actions->close ();
    SavedSearches::destruct ();

# ifndef DISABLE_PLUGINS
    if (plugin_manager->astroid_extension) delete plugin_manager->astroid_extension;
    if (plugin_manager) delete plugin_manager;
# endif

    if (logf.is_open()) {
      // TODO: log
      logf.close ();
    }

    LOG (info) << "astroid: goodbye!";
  }

  Astroid::~Astroid () {
    /* this is only run for tests */
    delete accounts;
    delete m_config;
    delete poll;

    if (actions) actions->close ();
    delete actions;

    // TODO: log
    /* log.del_out_stream (&cout); */
  }

  MainWindow * Astroid::open_new_window (bool open_defaults) {
    LOG (warn) << "astroid: starting a new window..";

    /* set up a new main window */

    /* start up default window with default buffers */
    MainWindow * mw = new MainWindow (); // is freed / destroyed by application

    if (open_defaults) {
      if (config ("saved_searches").get<bool>("show_on_startup")) {
        Mode * s = (Mode *) new SavedSearches (mw);
        s->invincible = true;
        mw->add_mode (s);
      }

      ptree qpt = config ("startup.queries");

      for (const auto &kv : qpt) {
        ustring name = kv.first;
        ustring query = kv.second.data();

        LOG (info) << "astroid: got query: " << name << ": " << query;
        Mode * ti = new ThreadIndex (mw, query, name);
        ti->invincible = true; // set startup queries to be invincible
        mw->add_mode (ti);
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
    LOG (info) << "astorid: mailto: " << url;

    ustring scheme = Glib::uri_parse_scheme (url);
    if (scheme.length () > 0) {
      /* we got an mailto url */
      url = url.substr(scheme.length(), url.length () - scheme.length());

      ustring to, cc, bcc, subject, body;

      ustring::size_type pos = url.find ("?");
      /* ustring::size_type next; */
      if (pos == ustring::npos) pos = url.length ();
      to = url.substr (0, pos);

      /* TODO: need to finish the rest of the fields */
      mw->add_mode (new EditMessage (mw, to));

    } else {
      /* we probably just got the address on the cmd line */
      mw->add_mode (new EditMessage (mw, url));
    }
  }

}

