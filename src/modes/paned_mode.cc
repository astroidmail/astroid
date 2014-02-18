# include <iostream>
# include <vector>
# include <exception>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "mode.hh"
# include "paned_mode.hh"

using namespace std;

namespace Gulp {

  PanedMode::PanedMode () {
    pack_start (paned, true, true, 5);
    paned.show_all ();
  }

  void PanedMode::add_pane (int p, Mode &w) {
    cout << "pm: add pane" << endl;

    if (packed >= 2) {
      throw out_of_range ("Can only embed two panes.");
    }


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
    cout << "pm: del pane" << endl;

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
    cout << "pm: grab modal to: " << current << endl;
    if (current == 0) {
      pw1->grab_modal ();
      has_modal = true;
    } else if (current == 1) {
      pw2->grab_modal ();
      has_modal = true;
    }
  }

  void PanedMode::release_modal () {
    cout << "pm: release modal: " << current << " (" << has_modal << ")" << endl;
    if (current == 0) {
      pw1->release_modal ();
    } else if (current == 1) {
      pw2->release_modal ();
    }
    has_modal = false;
  }
}

