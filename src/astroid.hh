# pragma once

# include <vector>
# include <string>
# include <atomic>
# include <fstream>

# include <boost/property_tree/ptree.hpp>
# include <boost/program_options.hpp>
# include <boost/log/trivial.hpp>

# define LOG(x) BOOST_LOG_TRIVIAL(x)
# define warn warning

# include <gtkmm.h>
# include <glibmm.h>

# include "proto.hh"

namespace po = boost::program_options;

namespace Astroid {
  class Astroid : public Gtk::Application {
    public:
      Astroid ();
      ~Astroid ();

      static refptr<Astroid> create ();

      int run (int, char**);
      void main_test ();
      void init_log ();

      const boost::property_tree::ptree& config (const std::string& path=std::string()) const;
      const boost::property_tree::ptree& notmuch_config () const;
      const StandardPaths& standard_paths() const;
            RuntimePaths& runtime_paths() const;
      bool  in_test ();

      static const char* const version;
      ustring user_agent;

      /* accounts */
      AccountManager * accounts;

      /* actions */
      ActionManager * actions;

# ifndef DISABLE_PLUGINS
      PluginManager * plugin_manager;
# endif

      /* poll */
      Poll * poll;

      MainWindow * open_new_window (bool open_defaults = true);

      int hint_level ();

    protected:
      Config * m_config;

    private:
      void on_activate () override;
      bool on_window_close (GdkEventAny *, MainWindow * mw);
      int  on_command_line (const refptr<Gio::ApplicationCommandLine> &) override;
      void on_quit ();

      void send_mailto (ustring);


      static std::atomic<bool> log_initialized;
      int _hint_level = 0;
      po::options_description desc;
  };

  /* globally available instance of our main Astroid-class */
  extern refptr<Astroid> astroid;
}

