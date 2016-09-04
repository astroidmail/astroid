# pragma once

# include <vector>
# include <string>
# include <atomic>
# include <fstream>

# include <boost/property_tree/ptree.hpp>
# include <boost/log/trivial.hpp>

# define LOG(x) BOOST_LOG_TRIVIAL(x)
# define warn warning

# include <gtkmm.h>
# include <glibmm.h>

# include "proto.hh"

namespace Astroid {
  class Astroid {
    public:
      Astroid ();
      ~Astroid ();
      int main (int, char**);
      void main_test ();
      void init_log ();

      const boost::property_tree::ptree& config (const std::string& path=std::string()) const;
      const boost::property_tree::ptree& notmuch_config () const;
      const StandardPaths& standard_paths() const;
      bool  in_test ();

      refptr<Gtk::Application> app;

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
      bool activated = false;
      void on_signal_activate ();
      void on_mailto_activate (const Glib::VariantBase &);
      refptr<Gio::SimpleAction> mailto;
      void send_mailto (MainWindow * mw, ustring);

      void on_quit ();

      static std::atomic<bool> log_initialized;
      int _hint_level = 0;
  };

  /* globally available instance of our main Astroid-class */
  extern Astroid * astroid;
}

