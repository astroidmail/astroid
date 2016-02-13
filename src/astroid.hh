# pragma once

# include <vector>
# include <string>

# include <boost/property_tree/ptree.hpp>

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

      const boost::property_tree::ptree& config (const std::string& path=std::string()) const;
      const boost::property_tree::ptree& notmuch_config () const;
      const StandardPaths& standard_paths() const;
      bool  in_test ();

      /* this is true if there has been a fatal error, this means
       * that nothing more really can happen except closing astroid.
       *
       * a window with the fatal error is shown plus a log window */
      bool in_failure ();
      void fail (ustring);

      refptr<Gtk::Application> app;

      static const char* const version;
      ustring user_agent;

      /* accounts */
      AccountManager * accounts;

      /* actions */
      GlobalActions * global_actions;

      /* poll */
      Poll * poll;

      MainWindow * open_new_window (bool open_defaults = true);

    protected:
      Config * m_config;
      bool     _in_failure;
      bool     _failmode_shown;
      bool     _is_main_window_ready;
      ustring  fail_str;

    private:
      bool activated = false;
      void on_signal_activate ();
      void on_mailto_activate (const Glib::VariantBase &);
      refptr<Gio::SimpleAction> mailto;
      void send_mailto (MainWindow * mw, ustring);
  };

  /* globally available instance of our main Astroid-class */
  extern Astroid * astroid;
}

