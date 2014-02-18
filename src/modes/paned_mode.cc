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
  }

  void PanedMode::add_pane (Mode &w) {
    paned_widgets.push_back (&w);

    Gtk::Widget * ww = &w;

    paned.add2 (*ww);

    if (current <= 0)
      current = paned_widgets.size() -1;
  }

  void PanedMode::del_pane (Mode &w) {
    cout << "pm: del pane" << endl;
    auto it = find (paned_widgets.begin (), paned_widgets.end (), &w);
    if ((it == paned_widgets.end ()) && *it != &w) {
      throw invalid_argument ("The widget is not among the paned");
    }

    paned.remove (*( (Gtk::Widget*) (*it)));
    auto newe = paned_widgets.erase (it);

    int i = newe - paned_widgets.begin ();

    if ((i+1) == current) {
      current = i;
      if (has_modal) grab_modal ();
    }
  }

  void PanedMode::grab_modal () {
    if (current >= 0) {
      if (paned_widgets.size() > current) {
        cout << "pm: grab modal to: " << current << endl;
        paned_widgets[current]->grab_modal ();
        has_modal = true;
      }
    }
  }

  void PanedMode::release_modal () {
    if (current >= 0) {
      if (paned_widgets.size() > current) {
        cout << "pm: release modal: " << current << " (" << has_modal << ")" << endl;

        paned_widgets[current]->release_modal ();
      }
    }
    has_modal = false;
  }
}

