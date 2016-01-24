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
      const StandardPaths& standard_paths() const;

      refptr<Gtk::Application> app;

      static const char* const version;
      ustring user_agent;

      /* accounts */
      AccountManager * accounts;

      /* contacts */
      //Contacts * contacts;

      /* actions */
      GlobalActions * global_actions;

      /* poll */
      Poll * poll;

      MainWindow * open_new_window (bool open_defaults = true);

    protected:
      Config * m_config;

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

