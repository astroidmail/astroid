# pragma once

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"

namespace Astroid {
  class FailMode : public Mode {
    public:
      FailMode (MainWindow *, ustring err);

      Gtk::ScrolledWindow scroll;
      Gtk::Label fail_text;
      LogView * lv;

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

