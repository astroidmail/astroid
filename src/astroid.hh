# pragma once

# include <vector>
# include <string>
# include <atomic>
# include <fstream>

# include <boost/property_tree/ptree.hpp>
# include <boost/program_options.hpp>
# include <boost/log/trivial.hpp>
# include <boost/filesystem.hpp>

# define LOG(x) BOOST_LOG_TRIVIAL(x)
# define warn warning

# include <gmime/gmime.h>

# include <gtkmm.h>
# include <glibmm.h>

# include "proto.hh"

namespace po      = boost::program_options;
namespace logging = boost::log;

namespace Astroid {
  class Astroid : public Gtk::Application {
    public:
      Astroid ();
      ~Astroid ();

      static refptr<Astroid> create ();

      int run (int, char**);
      void main_test ();

      void init_console_log ();
      void init_sys_log ();
      bool log_stdout   = true;
      bool log_syslog   = false;
      bool disable_log  = false;

      ustring log_level = "debug";
      std::map<std::string, logging::trivial::severity_level> sevmap = {
        std::pair<std::string,logging::trivial::severity_level>("trace"  , logging::trivial::trace),
        std::pair<std::string,logging::trivial::severity_level>("debug"  , logging::trivial::debug),
        std::pair<std::string,logging::trivial::severity_level>("info"   , logging::trivial::info),
        std::pair<std::string,logging::trivial::severity_level>("warning", logging::trivial::warning),
        std::pair<std::string,logging::trivial::severity_level>("error"  , logging::trivial::error),
        std::pair<std::string,logging::trivial::severity_level>("fatal"  , logging::trivial::fatal),
      };

      const std::string log_ident = "astroid.main";


      const boost::property_tree::ptree& config (const std::string& path=std::string()) const;
      const boost::filesystem::path& notmuch_config () const;
      const StandardPaths& standard_paths() const;
            RuntimePaths& runtime_paths() const;
      bool  has_notmuch_config ();
      bool  in_test ();

      static const char* const version;
      ustring user_agent;

      /* accounts */
      AccountManager * accounts = NULL;

      /* actions */
      ActionManager * actions = NULL;

# ifndef DISABLE_PLUGINS
      PluginManager * plugin_manager = NULL;
# endif

      /* poll */
      Poll * poll = NULL;

      MainWindow * open_new_window (bool open_defaults = true);

      int hint_level ();
      GdkAtom clipboard_target = GDK_SELECTION_CLIPBOARD;

    protected:
      Config * m_config = NULL;

    private:
      void on_activate () override;
      bool on_window_close (GdkEventAny *, MainWindow * mw);
      int  on_command_line (const refptr<Gio::ApplicationCommandLine> &) override;
      void on_quit ();

      void send_mailto (ustring);


      int _hint_level = 0;
      po::options_description desc;
      po::positional_options_description pdesc;
  };

  /* globally available instance of our main Astroid-class */
  extern refptr<Astroid> astroid;
}

