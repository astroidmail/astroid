# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/widget.h>

# include "mode.hh"

using namespace std;

namespace Gulp {
  /* a virtual mode class with two panes */
  class PanedMode : public Mode {
    public:
      PanedMode ();

      /* hpane: can be split into two horizontal panes */
      Gtk::Paned paned;
      Mode * pw1;
      Mode * pw2;
      int packed = 0;

      int  current = -1;
      void add_pane (int, Mode &w);
      void del_pane (int);

      bool has_modal = false;
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };

}
