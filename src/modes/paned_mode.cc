# include <iostream>
# include <vector>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "astroid.hh"
# include "mode.hh"
# include "paned_mode.hh"

using namespace std;

namespace Astroid {

  PanedMode::PanedMode (MainWindow * mw) : Mode (mw) {
    set_can_focus (true);
    add_events (Gdk::KEY_PRESS_MASK);

    pack_start (paned, true, true, 5);
    paned.show_all ();

    keys.title = "Paned mode";
  }

  void PanedMode::add_pane (int p, Mode &w) {
    LOG (debug) << "pm: add pane";

    if (packed >= 2) {
      throw out_of_range ("Can only embed two panes.");
    }

    Gtk::manage (&w);

    if (p == 0) pw1 = &w;
    else        pw2 = &w;

    Gtk::Widget * ww = &w;

    if (p == 0)
      paned.pack1 (*ww, true, false);
    else
      paned.pack2 (*ww, true, false);

    packed++;

    current = p;
  }

  void PanedMode::del_pane (int p) {
    LOG (debug) << "pm: del pane";

    if (p == current && has_modal) {
      release_modal ();
      current = -1;
    }

    if (p == 0) {
      paned.remove (*pw1);
      pw1 = NULL;
      current = 1;
      packed--;
    } else {
      paned.remove (*pw2);
      pw2 = NULL;
      current = 0;
      packed--;
    }

    if (packed < 1) {
      current = -1;
    } else {
      if (has_modal)
        grab_modal ();
    }

  }

  void PanedMode::grab_modal () {
    //LOG (debug) << "pm: grab modal to: " << current;
    if (current == 0) {
      pw1->grab_modal ();
      has_modal = true;
    } else if (current == 1) {
      pw2->grab_modal ();
      has_modal = true;
    }
  }

  void PanedMode::release_modal () {
    //LOG (debug) << "pm: release modal: " << current << " (" << has_modal << ")";
    if (current == 0) {
      pw1->release_modal ();
    } else if (current == 1) {
      pw2->release_modal ();
    }
    has_modal = false;
  }
}

