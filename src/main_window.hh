# pragma once

# include <atomic>

# include <gtkmm.h>
# include <gtkmm/window.h>
# include <gtkmm/notebook.h>

# include "proto.hh"
# include "command_bar.hh"
# include "modes/mode.hh"
# include "actions/action_manager.hh"

using namespace std;

namespace Astroid {
  class Notebook : public Gtk::Notebook {
    public:
      Notebook ();

      void add_widget (Gtk::Widget *);
      void remove_widget (Gtk::Widget *);

    private:
      Gtk::HBox icons;
      Gtk::Spinner poll_spinner;
      bool spinner_on = false;

      void poll_state_changed (bool);
  };

  class MainWindow : public Gtk::Window {
    public:
      MainWindow ();
      int id;

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
      void close_page ();

      void ungrab_active ();
      void grab_active (int);
      void set_active (int);
      void set_title (ustring);

      static ModeHelpInfo * key_help ();

      Glib::Dispatcher update_title_dispatcher;

    private:
      bool on_my_focus_in_event (GdkEventFocus *);
      bool on_my_focus_out_event (GdkEventFocus *);
      void on_my_switch_page (Gtk::Widget *, guint);

      void on_update_title ();

      static atomic<uint> nextid;
  };

}

