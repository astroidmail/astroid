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

      Gtk::ScrolledWindow scroll;
      Gtk::Label help_text;

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

