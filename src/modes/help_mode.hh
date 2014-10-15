# pragma once

# include <iostream>
# include <atomic>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  class HelpMode : public Mode {
    public:
      HelpMode ();
      HelpMode (Mode *);

      /* show help */
      void show_help (Mode * m);
      ustring generate_help (ModeHelpInfo *);

      Gtk::ScrolledWindow scroll;
      Gtk::Label help_text;

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;

      virtual ModeHelpInfo * key_help ();

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;

  };
}

