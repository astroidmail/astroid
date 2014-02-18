# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "gulp.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"

using namespace std;

namespace Gulp {
  MainWindow::MainWindow () {
    cout << "mw: init.." << endl;

    set_title ("Gulp");
    set_default_size (800, 400);

    add (notebook);

    show_all ();

    /* connect keys */
    add_events (Gdk::KEY_PRESS_MASK);
    signal_key_press_event ().connect (
        sigc::mem_fun(*this, &MainWindow::on_key_press));

  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    switch (event->keyval) {
      case GDK_KEY_q:
      case GDK_KEY_Q:
        gulp->quit ();
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
        cout << "mw: close page, not implemented" << endl;
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

  void MainWindow::set_active (int n) {
    cout << "mw: set active: " << n << ", current: " << current << endl;
    if (notebook.get_current_page() != n) {
      notebook.set_current_page (n);
    }

    if (current >= 0) {
      if (modes.size() > current) {
        cout << "mw: release modal, from: " << current << endl;
        modes[current]->release_modal ();
        current = -1;
      }
    }

    cout << "mw: grab modal to: " << n << endl;
    modes[n]->grab_modal ();
    current = n;
  }
}

