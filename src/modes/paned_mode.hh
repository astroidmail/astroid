# pragma once

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/widget.h>

# include "mode.hh"

namespace Astroid {
  /* a virtual mode class with two panes */
  class PanedMode : public Mode {
    public:
      PanedMode (MainWindow *);
      ~PanedMode ();

      /* hpane: can be split into two horizontal panes */
      Gtk::Paned    * paned;
      Gtk::EventBox * fp1 = NULL;
      Gtk::EventBox * fp2 = NULL;
      Mode * pw1 = NULL;
      Mode * pw2 = NULL;
      int packed = 0;

      int  current = -1;
      void add_pane (int, Mode * w);
      void del_pane (int);

      bool has_modal = false;
      virtual void grab_modal () override;
      virtual void release_modal () override;

      Gdk::RGBA selected_color;
  };

}
