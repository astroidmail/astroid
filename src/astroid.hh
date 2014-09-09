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
      void close (MainWindow *);

      refptr<Gtk::Application> app;

      ustring user_agent;

      /* config */
      Config * config;

      /* accounts */
      AccountManager * accounts;

      /* contacts */
      Contacts * contacts;

      /* actions */
      GlobalActions * global_actions;

      /* poll */
      Poll * poll;

      MainWindow * open_new_window ();

    private:
      bool activated = false;
      void on_signal_activate ();
  };

  /* globally available instance of our main Astroid-class */
  extern Astroid * astroid;
}

