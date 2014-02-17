# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/widget.h>

# include "mode.hh"

using namespace std;

namespace Gulp {
  /* modes that can be embedded as child panes should also inherit this
   * interface */
  class PanedChild {
    public:
      virtual void grab_modal () = 0;
      virtual void release_modal () = 0;
      virtual Gtk::Widget * get_widget () = 0;
  };

  /* a virtual mode class which can embed panes */
  class PanedMode : public Mode {
    public:
      PanedMode ();

      /* hpane: can be split into horizontal panes */
      Gtk::Paned paned;
      vector<PanedChild*> paned_widgets;

      int  current = -1;
      void add_pane (PanedChild &w);
      void del_pane (PanedChild &w);

      bool has_modal = false;
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };

}
