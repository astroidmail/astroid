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

    paned = Gtk::manage (new Gtk::Paned ());

    pack_start (*paned, true, true, 0);
    paned->show_all ();

    keys.title = "Paned mode";
    keys.register_key (Key (false, true, (guint) GDK_KEY_space), "pane.swap_focus",
        "Swap focus to other pane if open",
        [&] (Key) {
          if (packed == 2) {
            release_modal ();
            current = (current == 0 ? 1 : 0);
            grab_modal ();
          }
          return true;
        });

    /* color used for selected pane */
    selected_color = Gdk::RGBA ("#4a90d9");
  }

  PanedMode::~PanedMode () {
    LOG (debug) << "pm: deconstruct";
  }

  void PanedMode::add_pane (int p, Mode * w) {
    LOG (debug) << "pm: add pane";

    if (packed >= 2) {
      throw out_of_range ("Can only embed two panes.");
    }
    if (p == 0) pw1 = w;
    else        pw2 = w;

    w = Gtk::manage (w);

    Gtk::EventBox * fb = Gtk::manage (new Gtk::EventBox ());
    fb->add (*w);

    if (p == 0) {
      fp1 = fb;
      paned->pack1 (*fb, true, false);
    } else {
      fp2 = fb;
      paned->pack2 (*fb, true, false);
    }

    packed++;

    if (packed > 1) {
      pw1->set_margin_top (5);
      pw2->set_margin_top (5);
    }

    paned->show_all ();

    current = p;
  }

  void PanedMode::del_pane (int p) {
    LOG (debug) << "pm: del pane: " << p;

    if (p == current && has_modal) {
      release_modal ();
      current = -1;
      has_modal = true;
    }

    if (p == 0) {
      paned->remove (*fp1);

      delete fp1;
      delete pw1;

      pw1 = NULL;
      fp1 = NULL;
      current = 1;
      packed--;
      if (pw2 != NULL) {
        pw2->set_margin_top (0);
      }
    } else {
      paned->remove (*fp2);
      if (pw1 != NULL) {
        pw1->set_margin_top (0);
      }

      delete fp2;
      delete pw2;

      fp2 = NULL;
      pw2 = NULL;
      current = 0;
      packed--;
    }

    if (packed < 1) {
      current = -1;
      has_modal = false;
    } else {
      if (has_modal) {
        grab_modal ();
      }
    }
  }

  void PanedMode::grab_modal () {
    //LOG (debug) << "pm: grab modal to: " << current;
    if (current == 0) {
      pw1->grab_modal ();
      if (pw2 != NULL) {
        fp1->override_background_color (selected_color, Gtk::StateFlags::STATE_FLAG_NORMAL);
        fp2->override_background_color (Gdk::RGBA (), Gtk::StateFlags::STATE_FLAG_NORMAL);
      }
      has_modal = true;
    } else if (current == 1) {
      pw2->grab_modal ();
      if (pw1 != NULL) {
        fp2->override_background_color (selected_color, Gtk::StateFlags::STATE_FLAG_NORMAL);
        fp1->override_background_color (Gdk::RGBA (), Gtk::StateFlags::STATE_FLAG_NORMAL);
      }
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

