# pragma once

# include <atomic>
# include <functional>

# include <gtkmm.h>
# include <gtkmm/window.h>
# include <gtkmm/notebook.h>

# include "proto.hh"
# include "command_bar.hh"
# include "modes/mode.hh"
# include "modes/keybindings.hh"
# include "actions/action_manager.hh"

namespace Astroid {
  class Notebook : public Gtk::Notebook {
    public:
      Notebook ();

      void add_widget (Gtk::Widget *);
      void remove_widget (Gtk::Widget *);

      /* height of action widget */
      static int icon_size;

    private:
      Gtk::HBox icons;
      Gtk::Spinner poll_spinner;
      bool spinner_on = false;

      void poll_state_changed (bool);

      void on_my_size_allocate (Gtk::Allocation &);
  };

  class MainWindow : public Gtk::Window {
    public:
      MainWindow ();
      int id;

      bool on_key_press (GdkEventKey *);
      bool mode_key_handler (GdkEventKey *);

      Gtk::Box vbox;
      Notebook notebook;

      /* command bar */
      CommandBar command;

      bool is_command = false;
      void enable_command (CommandBar::CommandMode, ustring,
          std::function<void(ustring)>);
      void enable_command (CommandBar::CommandMode, ustring, ustring,
          std::function<void(ustring)>);
      void disable_command ();
      void on_command_mode_changed ();

      /* actions */
      ActionManager * actions;

      int current = -1;
      bool active = false;

      void add_mode (Mode *);
      void del_mode (int);
      void remove_all_modes ();
      void close_page (bool = false);
      void close_page (Mode *, bool = false);
      bool jump_to_page (Key, int);

      void ungrab_active ();
      void grab_active (int);
      void set_active (int);
      void set_title (ustring);

      void quit ();

      Glib::Dispatcher update_title_dispatcher;

      Keybindings keys;

    private:
      bool on_my_focus_in_event (GdkEventFocus *);
      bool on_my_focus_out_event (GdkEventFocus *);
      void on_my_switch_page (Gtk::Widget *, guint);

      void on_update_title ();

      static std::atomic<uint> nextid;

      bool in_quit = false;


      /* YES-NO questions */
      Gtk::Revealer * rev_yes_no;
      Gtk::Label    * label_yes_no;

      bool yes_no_waiting = false;
      std::function <void (bool)> yes_no_closure = NULL;
      void answer_yes_no (bool);
      void on_yes ();
      void on_no ();

      /* multi key */
      Gtk::Revealer * rev_multi;
      Gtk::Label    * label_multi;

      bool multi_waiting  = false;
      Keybindings multi_keybindings;

    public:
      void ask_yes_no (ustring, std::function<void(bool)>);
      bool multi_key (Keybindings &, Key);

  };

}

