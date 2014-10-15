# pragma once

# include <gtkmm.h>
# include <gtkmm/window.h>

# include "proto.hh"
# include "command_bar.hh"
# include "modes/mode.hh"
# include "actions/action_manager.hh"

using namespace std;

namespace Astroid {
  class Notebook : public Gtk::Notebook {

  };

  class MainWindow : public Gtk::Window {
    public:
      MainWindow ();

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

      int current = -1;
      bool active = false;

      void add_mode (Mode *);
      void del_mode (int);
      void remove_all_modes ();

      void unset_active ();
      void set_active (int);

      static ModeHelpInfo * key_help ();

    private:
      bool on_my_focus_in_event (GdkEventFocus *);
      bool on_my_focus_out_event (GdkEventFocus *);
  };

}

