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
# include <boost/log/trivial.hpp>
# include <boost/log/support/date_time.hpp>
# include <boost/log/sinks/syslog_backend.hpp>

# include "astroid.hh"
# include "build_config.hh"
# include "db.hh"
# include "config.hh"
# include "account_manager.hh"
# include "actions/action_manager.hh"
# include "actions/action.hh"
# include "utils/date_utils.hh"
# include "utils/utils.hh"
# include "utils/resource.hh"

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
# include <utils/gmime/gmime-compat.h>

using namespace std;
using namespace boost::filesystem;

namespace logging   = boost::log;
namespace keywords  = boost::log::keywords;
namespace expr      = boost::log::expressions;

/* globally available static instance of the Astroid */
refptr<Astroid::Astroid> Astroid::astroid;
const char* const Astroid::Astroid::version = GIT_DESC;

namespace Astroid {
  // Initialization and creation {{{
  refptr<Astroid> Astroid::create () {
    return refptr<Astroid> (new Astroid ());
  }

  void Astroid::init_console_log () {
    /* log to console */
    logging::formatter format =
                  expr::stream
                      << "["
                      << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S")
                      << "] [" << expr::attr <boost::log::attributes::current_thread_id::value_type>("ThreadID")
                      << "] [M] [" << logging::trivial::severity
                      << "] " << expr::smessage
              ;

    logging::add_console_log ()->set_formatter (format);
  }

  void Astroid::init_sys_log () {
    typedef logging::sinks::synchronous_sink< logging::sinks::syslog_backend > sink_t;

    // Create a backend
    boost::shared_ptr< logging::sinks::syslog_backend > backend(new logging::sinks::syslog_backend(
          keywords::facility = logging::sinks::syslog::user,
          keywords::use_impl = logging::sinks::syslog::native,
          keywords::ident    = log_ident
          ));

    // Set the straightforward level translator for the "Severity" attribute of type int
    backend->set_severity_mapper(
        logging::sinks::syslog::direct_severity_mapping< int >("Severity"));

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    logging::core::get()->add_sink(boost::make_shared< sink_t >(backend));
  }

  Astroid::Astroid () :
    Gtk::Application("org.astroid",
        Gio::APPLICATION_HANDLES_OPEN | Gio::APPLICATION_HANDLES_COMMAND_LINE)
  {
    setlocale (LC_ALL, "");
    Glib::init ();

    /* user agent */
    user_agent = ustring::compose ("astroid/%1 (https://github.com/astroidmail/astroid)", Astroid::version);

    /* gmime settings */
    g_mime_init ();

    /* options */
    desc.add_options ()
      ( "help,h", "print this help message")
      ( "config,c", po::value<ustring>(), "config file, default: $XDG_CONFIG_HOME/astroid/config")
      ( "new-config,n",   "make new default config, then exit")
# ifdef DEBUG
      ( "test-config,t",  "use test config (same as used when tests are run), only makes sense from the source root")
# endif
      ( "mailto,m", po::value<ustring>(), "compose mail with mailto url or address")
      ( "no-auto-poll",   "do not poll automatically")
      ( "disable-log",    "disable logging")
      ( "log-stdout",     "log to stdout regardless of configuration")
      ( "start-polling",  "indicate that external polling (external notmuch db R/W operations) starts")
      ( "stop-polling",   "indicate that external polling stops")
      ( "refresh", po::value<unsigned long>(), "refresh messages changed since lastmod")
# ifndef DISABLE_PLUGINS
      ( "disable-plugins", "disable plugins");
# else
      ;
# endif
  }
  // }}}

  int Astroid::run (int argc, char **argv) { // {{{
    register_application ();

    Resource::init (argv[0]);

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

    logging::add_common_attributes ();
    if (vm.count ("log-stdout")) {
      log_stdout = true;
      init_console_log ();
    }

    if (vm.count ("disable-log")) {
      logging::core::get()->set_logging_enabled (false);
      disable_log = true;
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
      ncnf.write_back_config ();

      exit (0);
    } // }}}

    if (!is_remote ()) {
      /* load config */
      if (vm.count("config")) {
        if (test_config) {
          cout << "--config cannot be specified together with --test-config.";
          exit (1);
        }

        cout << "astroid: loading config: " << vm["config"].as<ustring>().c_str();
        m_config = new Config (vm["config"].as<ustring>().c_str());
      } else {
        if (test_config) {
          m_config = new Config (true);
        } else {
          m_config = new Config ();
        }
      }

      /* setting up loggers */
      if (config ("astroid.log").get<bool>("stdout") && !vm.count ("log-stdout")) {
        init_console_log ();
        log_stdout = true;
      }

      if (config ("astroid.log").get<bool> ("syslog")) {
        init_sys_log ();
        log_syslog = true;
      }

      log_level = config ("astroid.log").get<std::string> ("level");

      /* Non existing llevel in map will be silently ignored */
      logging::core::get()->set_filter (logging::trivial::severity >= sevmap[log_level]);

      LOG (info) << "welcome to astroid! - " << Astroid::version;

      string charset;
      Glib::get_charset(charset);
      LOG (debug) << "utf8: " << Glib::get_charset () << ", " << charset;

      if (!Glib::get_charset()) {
        LOG (error) << "astroid needs an UTF-8 locale! this is probably not going to work.";
      }


      if (vm.count("start-polling") || vm.count("stop-polling") || vm.count ("refresh")) {
        LOG (error) << "--start-polling, --stop-polling or --refresh can only be specifed when there is already a running astroid instance.";
        exit (1);
      }

      _hint_level = config ("astroid.hints").get<int> ("level");

      /* set up classes */
      Date::init ();
      Utils::init ();

      /* Initialize Db and check if it has been set up */
      try {
        Db::init ();
        Db d; d.get_revision ();
      } catch (database_error &ex) {
        LOG (error) << "db: failed to open database, please check the manual if everything is set up correctly: " << ex.what ();

        Gtk::MessageDialog md ("Astroid: Failed to open database", false, Gtk::MESSAGE_ERROR);
        md.set_secondary_text (ustring::compose ("Please check the documentation if you have set up astroid and notmuch correctly.\n Error: <b>%1</b>", ex.what ()), true);

        md.run ();

        on_quit ();
        return 1;
      }

      Keybindings::init ();
      SavedSearches::init ();

      /* set up accounts */
      accounts = new AccountManager ();

# ifndef DISABLE_PLUGINS
      /* set up plugins */
      bool disable_plugins = vm.count ("disable-plugins");
      plugin_manager = new PluginManager (disable_plugins, in_test ());
      plugin_manager->astroid_extension = new PluginManager::AstroidExtension (this);
# endif

      /* set up global actions */
      actions = new ActionManager ();

      /* set up poller */
      bool no_auto_poll = false;
      if (vm.count ("no-auto-poll")) {
        LOG (info) << "astroid: automatic polling is off.";

        no_auto_poll = true;
      }
      poll = new Poll (!no_auto_poll);

      Gtk::Application::run (argc, argv);

      on_quit ();

    } else {
      Gtk::Application::run (argc, argv);

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

  bool Astroid::has_notmuch_config () {
    return m_config->has_notmuch_config;
  }

  void Astroid::main_test () { // {{{
    init_console_log ();

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

    if (poll && poll->get_auto_poll ()) poll->toggle_auto_poll  ();
    if (poll) poll->close ();

    if (actions) actions->close ();
    SavedSearches::destruct ();

# ifndef DISABLE_PLUGINS
    if (plugin_manager && plugin_manager->astroid_extension) delete plugin_manager->astroid_extension;
    if (plugin_manager) delete plugin_manager;
# endif

    LOG (info) << "astroid: goodbye!";
  }

  Astroid::~Astroid () {
    if (accounts) delete accounts;

    if (m_config) delete m_config;

    if (poll) {
      poll->close ();
      delete poll;
    }

    if (actions) {
      actions->close ();
      delete actions;
    }
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

        LOG (info) << "got query: " << name << ": " << query;
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

