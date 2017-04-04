# include <iostream>
# include <vector>
# include <memory>
# include <atomic>

# include <gtkmm.h>
# include <gtkmm/window.h>

/* program options */
# include <boost/program_options.hpp>
# include <boost/filesystem.hpp>

/* log */
# include <boost/log/core.hpp>
# include <boost/log/utility/setup/file.hpp>
# include <boost/log/utility/setup/console.hpp>
# include <boost/log/utility/setup/common_attributes.hpp>
# include <boost/log/sinks/text_file_backend.hpp>
# include <boost/log/sources/severity_logger.hpp>
# include <boost/log/sources/record_ostream.hpp>
# include <boost/log/utility/setup/filter_parser.hpp>
# include <boost/log/utility/setup/formatter_parser.hpp>
# include <boost/date_time/posix_time/posix_time_types.hpp>
# include <boost/log/expressions.hpp>
# include <boost/log/support/date_time.hpp>

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

namespace logging   = boost::log;
namespace keywords  = boost::log::keywords;
namespace expr      = boost::log::expressions;

/* globally available static instance of the Astroid */
refptr<Astroid::Astroid> Astroid::astroid;
const char* const Astroid::Astroid::version = GIT_DESC;
std::atomic<bool> Astroid::Astroid::log_initialized (false);

namespace Astroid {
  // Initialization and creation {{{
  refptr<Astroid> Astroid::create () {
    return refptr<Astroid> (new Astroid ());
  }

  void Astroid::init_log () {
    if (log_initialized) return;
    log_initialized = true;
    /* log to console */
    logging::add_common_attributes ();
    logging::formatter format =
                  expr::stream
                      << "["
                      << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S.%f")
                      << "] [" << expr::attr <boost::log::attributes::current_thread_id::value_type>("ThreadID")
                      << "] [" << logging::trivial::severity
                      << "] " << expr::smessage
              ;

    logging::add_console_log ()->set_formatter (format);
  }

  Astroid::Astroid () :
    Gtk::Application("org.astroid",
        Gio::APPLICATION_HANDLES_OPEN | Gio::APPLICATION_HANDLES_COMMAND_LINE)
  {
    setlocale (LC_ALL, "");
    Glib::init ();
    init_log ();

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

    /* options */
    desc.add_options ()
      ( "help,h", "print this help message")
      ( "config,c", po::value<ustring>(), "config file, default: $XDG_CONFIG_HOME/astroid/config")
      ( "new-config,n", "make new default config, then exit")
      ( "test-config,t", "use test config (same as used when tests are run), only makes sense from the source root")
      ( "mailto,m", po::value<ustring>(), "compose mail with mailto url or address")
      ( "no-auto-poll", "do not poll automatically")
      ( "log,l", po::value<ustring>(), "log to file")
      ( "append-log,a", "append to log file")
      ( "start-polling", "indicate that external polling (external notmuch db R/W operations) starts")
      ( "stop-polling", "indicate that external polling stops")
      ( "refresh", po::value<unsigned long>(), "refresh threads changed since lastmod")
# ifndef DISABLE_PLUGINS
      ( "disable-plugins", "disable plugins");
# else
      ;
# endif
  }
  // }}}

  int Astroid::run (int argc, char **argv) { // {{{
    register_application ();

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

    /* log to file */
    if (vm.count ("log")) {
      ustring lfile = vm["log"].as<ustring> ();
      bool append = vm.count ("append-log");

      std::ios_base::openmode om = std::ofstream::out;
      if (append) om |= std::ofstream::app;

      logging::add_file_log (
          keywords::file_name   = lfile.c_str (),
          keywords::auto_flush  = true,
          keywords::open_mode   = om,

          keywords::format =
                (
                  expr::stream
                      << "["
                      << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                      << "] [" << expr::attr <boost::log::attributes::current_thread_id::value_type>("ThreadID")
                      << "] [" << logging::trivial::severity
                      << "] " << expr::smessage
              )
          );

      LOG (info) << "logging to: " << lfile << " (append: " << append << ")";
    }


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

    if (!is_remote ()) {
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

      if (vm.count("start-polling") || vm.count("stop-polling") || vm.count ("refresh")) {
        LOG (error) << "--start-polling, --stop-polling or --refresh can only be specifed when there is already a running astroid instance.";
        exit (1);
      }

      _hint_level = config ("astroid.hints").get<int> ("level");

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

      Gtk::Application::run (argc, argv);

      on_quit ();

    } else {
      Gtk::Application::run (argc, argv);

      if (no_auto_poll) {
        LOG (warn) << "astroid: specifying no-auto-poll only makes sense when starting a new astroid instance, ignoring.";
      }
    }

    return 0;
  } // }}}

  const boost::property_tree::ptree& Astroid::config (const std::string& id) const {
    return m_config->config.get_child(id);
  }

  const boost::property_tree::ptree& Astroid::notmuch_config () const {
    return m_config->notmuch_config;
  }

  const StandardPaths& Astroid::standard_paths() const {
    return m_config->std_paths;
  }

  RuntimePaths& Astroid::runtime_paths() const {
    return m_config->run_paths;
  }

  void Astroid::main_test () { // {{{
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
  } // }}}

  bool Astroid::in_test () {
    return m_config->test;
  }

  void Astroid::on_quit () {
    LOG (debug) << "astroid: quitting..";

    if (poll->get_auto_poll ()) poll->toggle_auto_poll  ();
    if (poll) poll->close ();

    if (actions) actions->close ();
    SavedSearches::destruct ();

# ifndef DISABLE_PLUGINS
    if (plugin_manager->astroid_extension) delete plugin_manager->astroid_extension;
    if (plugin_manager) delete plugin_manager;
# endif

    LOG (info) << "astroid: goodbye!";
  }

  Astroid::~Astroid () {
    delete accounts;
    delete m_config;

    if (poll) poll->close ();
    delete poll;

    if (actions) actions->close ();
    delete actions;
  }

  int Astroid::on_command_line (const refptr<Gio::ApplicationCommandLine> & cmd) {
    char ** argv;
    int     argc;
    bool new_window = true;

    argv = cmd->get_arguments (argc);

    if (get_windows().empty ()) {
      activate ();
      new_window = false;
    }

    /* handling command line arguments that may be passed from secondary
     * instances as well */
    if (argc > 0) {
      po::variables_map vm;

      try {
        po::store ( po::parse_command_line (argc, argv, desc), vm );
      } catch (po::unknown_option &ex) {
        cout << "unknown option" << endl;
        cout << ex.what() << endl;
        return 1;
      }

      if (vm.count("mailto")) {
        ustring mailtourl = vm["mailto"].as<ustring>();
        send_mailto (mailtourl);
        new_window = false;
      }

      if ((vm.count ("start-polling") ? 1:0) + (vm.count ("stop-polling") ? 1:0) + (vm.count("refresh") ? 1:0) > 1) {
          LOG (error) << "only one of --start-polling, --stop-polling and --refresh should be specified";
          return 1;
      }

      if (vm.count ("start-polling")) {

        poll->start_polling ();
        new_window = false;

      } else if (vm.count ("stop-polling")) {

        poll->stop_polling ();
        new_window = false;

      } else if (vm.count ("refresh")) {
        unsigned long last = vm["refresh"].as<unsigned long> ();
        poll->refresh ( last );
        new_window = false;
      }
    }

    if (new_window) activate ();

    return 0;
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

    mw->signal_delete_event ().connect (
        sigc::bind (
          sigc::mem_fun(*this, &Astroid::on_window_close), mw));

    add_window (*mw);
    mw->show_all ();

    return mw;
  }

  bool Astroid::on_window_close (GdkEventAny *, MainWindow * mw) {
    // application automatically removes window

    // https://mail.gnome.org/archives/gtkmm-list/2004-March/msg00282.html
    delete mw;

    return false; // default Gtk handler destroys window
  }

  void Astroid::on_activate () {
    open_new_window ();
  }

  void Astroid::send_mailto (ustring url) {
    LOG (info) << "astroid: mailto: " << url;

    MainWindow * mw = (MainWindow*) get_windows ()[0];

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

  int Astroid::hint_level () {
    return _hint_level;
  }

}

