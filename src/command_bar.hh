/* command bar
 *
 * is sub classed from Gtk::SearchBar, provides a command and search bar
 * with an entry that takes completions.
 *
 * will in different modes accept full searches, buffer searches, commands
 * or arguments for commands.
 *
 */

# pragma once

# include <gtkmm.h>

# include <functional>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class CommandBar : public Gtk::SearchBar {
    public:
      enum CommandMode {
        Search = 0,
        Generic,
        Tag,        /* apply or remove tags */
      };

      CommandBar ();

      MainWindow * main_window;
      CommandMode mode;

      Gtk::Box hbox;
      Gtk::Label mode_label;
      Gtk::SearchEntry entry;

      void on_entry_activated ();
      function<void(ustring)> callback;

      void enable_command (CommandMode, ustring, function<void(ustring)>);
      void disable_command ();

      void handle_command (ustring);

      ustring get_text ();
      void set_text (ustring);

      /* relay to search bar event handler */
      bool command_handle_event (GdkEventKey *);


  };
}
