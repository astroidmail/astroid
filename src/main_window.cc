# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "astroid.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"

using namespace std;

namespace Astroid {
  MainWindow::MainWindow () {
    cout << "mw: init.." << endl;

    set_title ("Astroid");
    set_default_size (800, 400);

    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "mail-send-symbolic", 42, Gtk::ICON_LOOKUP_USE_BUILTIN );
    set_icon (pixbuf);

    vbox.set_orientation (Gtk::ORIENTATION_VERTICAL);
    s_hbox.set_orientation (Gtk::ORIENTATION_HORIZONTAL);

    sbar.set_show_close_button ();
    sbar.connect_entry (sentry);
    s_hbox.pack_start (sentry, Gtk::PACK_EXPAND_WIDGET, 5);
    sbar.add (s_hbox);

    sbar.property_search_mode_enabled().signal_changed().connect(
        sigc::mem_fun (*this, &MainWindow::on_search_mode_changed)
        );

    sentry.signal_activate ().connect (
        sigc::mem_fun (*this, &MainWindow::on_search_entry_activated)
        );

    vbox.pack_start (sbar, Gtk::PACK_SHRINK, 0);
    vbox.pack_start (notebook, Gtk::PACK_EXPAND_WIDGET, 0);

    add (vbox);

    show_all_children ();

    /* connect keys */
    add_events (Gdk::KEY_PRESS_MASK);
    signal_key_press_event ().connect (
        sigc::mem_fun(*this, &MainWindow::on_key_press));

  }

  void MainWindow::enable_search () {
    cout << "mw: enable search" << endl;
    sbar.set_search_mode (true);
    is_searching = true;
    unset_active ();
    sbar.add_modal_grab ();
  }

  void MainWindow::disable_search () {
    cout << "mw: disable search" << endl;
    // hides itself
    sbar.remove_modal_grab();
    set_active (lastcurrent);
    is_searching = false;
  }

  void MainWindow::on_search_entry_activated () {
    ustring stext = sentry.get_text ();
    cout << "mw: search for: " << stext << endl;
    sbar.set_search_mode (false); // emits changed -> disables search

    if (stext.size() > 0) {
      Mode * m = new ThreadIndex (this, stext);

      add_mode (m);
    }
  }

  void MainWindow::on_search_mode_changed () {
    cout << "mw: smode changed" << endl;
    if (!sbar.get_search_mode()) {
      disable_search ();
    }
  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    if (is_searching) {
      sbar.handle_event (event);
      return true;
    }

    switch (event->keyval) {
      case GDK_KEY_q:
      case GDK_KEY_Q:
        astroid->quit ();
        return true;

      /* page through notebook */
      case GDK_KEY_b:
        if (notebook.get_current_page () == (notebook.get_n_pages () - 1))
          set_active (0);
        else
          set_active (notebook.get_current_page() + 1);
        return true;

      case GDK_KEY_B:
        if (notebook.get_current_page() == 0)
          set_active (notebook.get_n_pages()-1);
        else
          set_active (notebook.get_current_page() - 1);

        return true;

      /* close page */
      case GDK_KEY_x:
        {
          int c = notebook.get_current_page ();
          del_mode (c);
        }
        return true;

      /* search */
      case GDK_KEY_F:
        enable_search ();
        return true;

    }
    return false;
  }

  MainWindow::~MainWindow () {
    cout << "mw: done." << endl;
  }

  void MainWindow::add_mode (Mode * m) {
    modes.push_back (m);
    Gtk::Widget * w = m;
    int n = notebook.append_page ((*w), *(m->tab_widget));
    notebook.show_all ();

    set_active (n);
  }

  void MainWindow::del_mode (int c) {
    notebook.remove_page (c);
    auto it = modes.begin () + c;
    modes.erase(it);
    c = min (c, notebook.get_n_pages()-1);
    set_active(c);
  }

  void MainWindow::unset_active () {
    if (current >= 0) {
      if (modes.size() > current) {
        cout << "mw: release modal, from: " << current << endl;
        modes[current]->release_modal ();
        lastcurrent = current;
        current = -1;
      }
    }
  }

  void MainWindow::set_active (int n) {
    cout << "mw: set active: " << n << ", current: " << current << endl;

    if (n >= 0 && n <= notebook.get_n_pages()-1) {

      if (notebook.get_current_page() != n) {
        notebook.set_current_page (n);
      }

      unset_active ();

      cout << "mw: grab modal to: " << n << endl;
      modes[n]->grab_modal ();
      current = n;
    } else {
      cout << "mw: set active: page is out of range: " << n << endl;
    }
  }
}

