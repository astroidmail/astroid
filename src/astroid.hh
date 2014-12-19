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
      ~Astroid ();
      int main (int, char**);
      void main_test ();

      refptr<Gtk::Application> app;

      ustring user_agent;

      /* config */
      Config * config;

      /* accounts */
      AccountManager * accounts;

      /* contacts */
      //Contacts * contacts;

      /* actions */
      GlobalActions * global_actions;

      /* poll */
      Poll * poll;

      MainWindow * open_new_window (bool open_defaults = true);

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

