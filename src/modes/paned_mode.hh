# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/widget.h>

# include "mode.hh"

using namespace std;

namespace Gulp {
  /* a virtual mode class which can embed panes */
  class PanedMode : public Mode {
    public:
      PanedMode ();

      /* hpane: can be split into horizontal panes */
      Gtk::Paned paned;
      vector<Mode*> paned_widgets;

      int  current = -1;
      void add_pane (Mode &w);
      void del_pane (Mode &w);

      bool has_modal = false;
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };

}
