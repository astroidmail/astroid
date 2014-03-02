# pragma once

# include <gtkmm.h>
# include <gtkmm/window.h>

# include "proto.hh"

using namespace std;

namespace Astroid {
  class Notebook : public Gtk::Notebook {

  };

  class MainWindow : public Gtk::Window {
    public:
      MainWindow ();
      ~MainWindow ();

      bool on_key_press (GdkEventKey *);

      Gtk::Box vbox;
      Notebook notebook;

      /* search bar */
      Gtk::SearchBar sbar;
      Gtk::SearchEntry sentry;
      Gtk::Box s_hbox;

      bool is_searching = false;
      void enable_search ();
      void disable_search ();
      void on_search_mode_changed ();
      void on_search_entry_activated ();

      vector<Mode*> modes;
      int lastcurrent = -1;
      int current = -1;

      void add_mode (Mode *);
      void del_mode (int);

      void unset_active ();
      void set_active (int);
  };

}

