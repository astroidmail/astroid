# pragma once

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"

namespace Astroid {
  class HelpMode : public Mode {
    public:
      HelpMode (MainWindow *);
      HelpMode (MainWindow *, Mode *);

      /* show help */
      void show_help (Mode * m);
      ustring generate_help (Gtk::Widget *);

      Gtk::ScrolledWindow scroll;
      Gtk::Label help_text;

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

