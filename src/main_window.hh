# pragma once

# include <gtkmm.h>
# include <gtkmm/window.h>

# include "proto.hh"
# include "command_bar.hh"
# include "actions/action_manager.hh"

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

      /* command bar */
      CommandBar command;

      bool is_command = false;
      void enable_command (CommandBar::CommandMode, ustring,
          function<void(ustring)>);
      void disable_command ();
      void on_command_mode_changed ();

      /* actions */
      ActionManager actions;


      vector<Mode*> modes;
      int lastcurrent = -1;
      int current = -1;

      void add_mode (Mode *);
      void del_mode (int);
      void remove_all_modes ();

      void unset_active ();
      void set_active (int);
  };

}

