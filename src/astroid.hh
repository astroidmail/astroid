# pragma once

# include <vector>
# include <memory>

# include <gtkmm.h>
# include <glibmm.h>

# include "proto.hh"

using namespace std;

namespace Astroid {
  class Astroid {
    public:
      Astroid ();
      int main (int, char**);
      void quit ();

      refptr<Gtk::Application> app;

      ustring user_agent;

      /* list of main windows */
      vector<MainWindow*>  main_windows;

      /* config */
      Config * config;

      /* accounts */
      AccountManager * accounts;

      /* contacts */
      Contacts * contacts;

      /* actions */
      GlobalActions * global_actions;
  };

  /* globally available instance of our main Astroid-class */
  extern Astroid * astroid;
}

