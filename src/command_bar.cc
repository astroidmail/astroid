# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "command_bar.hh"
# include "astroid.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"

using namespace std;

namespace Astroid {
  CommandBar::CommandBar () {
    set_show_close_button ();
    connect_entry (entry);

    hbox.set_orientation (Gtk::ORIENTATION_HORIZONTAL);

    hbox.pack_start (entry, Gtk::PACK_EXPAND_WIDGET, 5);
    add (hbox);

    entry.signal_activate ().connect (
        sigc::mem_fun (*this, &CommandBar::on_entry_activated)
        );

  }

  void CommandBar::on_entry_activated () {
    /* handle input */
    ustring cmd = get_text ();
    cout << "cb: cmd (in mode: " << mode << "): " << cmd << endl;
    set_search_mode (false); // emits changed -> disables search

    switch (mode) {
      case CommandMode::Search:
        if (cmd.size() > 0) {
          Mode * m = new ThreadIndex (main_window, cmd);

          main_window->add_mode (m);
        }
        break;

      case CommandMode::Generic:
        break;
    }
  }

  ustring CommandBar::get_text () {
    return entry.get_text ();
  }

  void CommandBar::set_text (ustring txt) {
    entry.set_text (txt);
  }

  void CommandBar::enable_command (CommandMode m, ustring cmd = "") {
    mode = m;
    entry.set_text (cmd);
    set_search_mode (true);
  }

  void CommandBar::disable_command () {

  }

  bool CommandBar::command_handle_event (GdkEventKey * event) {
    return handle_event (event);
  }
}

