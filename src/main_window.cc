# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>
# include <gtkmm/notebook.h>

# include "astroid.hh"
# include "build_config.hh"
# include "poll.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"
# include "modes/help_mode.hh"
# include "modes/edit_message.hh"
# include "modes/log_view.hh"
# include "command_bar.hh"
# include "actions/action.hh"
# include "actions/action_manager.hh"

using namespace std;

namespace Astroid {
  atomic<uint> MainWindow::nextid (0);

  Notebook::Notebook () {
    set_scrollable (true);

    set_action_widget (&icons, Gtk::PACK_END);
    icons.show_all ();

    astroid->poll->signal_poll_state ().connect (
        sigc::mem_fun (this, &Notebook::poll_state_changed));

    poll_state_changed (astroid->poll->get_poll_state());
  }

  void Notebook::poll_state_changed (bool state) {
    if (state && !spinner_on) {
      /* set up spinner for poll */
      spinner_on = true;
      icons.pack_end (poll_spinner, true, true, 5);
      icons.show_all ();
      poll_spinner.start ();
    } else if (!state && spinner_on) {
      poll_spinner.stop ();
      icons.remove (poll_spinner);
      spinner_on = false;
    }
  }

  void Notebook::add_widget (Gtk::Widget * w) {
    icons.pack_start (*w, true, true, 5);
    w->show ();
    icons.show_all ();
  }

  void Notebook::remove_widget (Gtk::Widget * w) {
    icons.remove (*w);
  }

  MainWindow::MainWindow () {
    id = ++nextid;

    log << debug << "mw: init, id: " << id << endl;

    set_title ("");
    set_default_size (1200, 800);

    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "mail-send-symbolic", 42, Gtk::ICON_LOOKUP_USE_BUILTIN );
    set_icon (pixbuf);

    vbox.set_orientation (Gtk::ORIENTATION_VERTICAL);

    command.set_main_window (this);

    command.property_search_mode_enabled().signal_changed().connect(
        sigc::mem_fun (*this, &MainWindow::on_command_mode_changed)
        );

    vbox.pack_start (command, false, true, 0);
    vbox.pack_start (notebook, Gtk::PACK_EXPAND_WIDGET, 0);

    add (vbox);

    show_all_children ();

    /* connect keys */
    add_events (Gdk::KEY_PRESS_MASK);
    signal_key_press_event ().connect (
        sigc::mem_fun(*this, &MainWindow::on_key_press));

    /* got focus, grab keys */
    signal_focus_in_event ().connect (
        sigc::mem_fun (this, &MainWindow::on_my_focus_in_event));
    signal_focus_out_event ().connect (
        sigc::mem_fun (this, &MainWindow::on_my_focus_out_event));

    /* change page */
    notebook.signal_switch_page ().connect (
        sigc::mem_fun (this, &MainWindow::on_my_switch_page));

    /* catch update title events */
    update_title_dispatcher.connect (
        sigc::mem_fun (this, &MainWindow::on_update_title));

  }

  void MainWindow::set_title (ustring t) {

    ustring tt = "Astroid (" GIT_DESC ")";

    if (t.size() > 0) {
      tt = t + " - " + tt;
    }

    Gtk::Window::set_title (tt);
  }

  void MainWindow::on_update_title () {
    int n = notebook.get_current_page();

    if (n >= 0 && n <= notebook.get_n_pages()-1) {
      set_title (((Mode*) notebook.get_nth_page(n))->get_label());
    } else {
      set_title ("");
    }
  }

  void MainWindow::enable_command (CommandBar::CommandMode m, ustring cmd, function<void(ustring)> f) {
    ungrab_active ();
    command.enable_command (m, cmd, f);
    is_command = true;
    command.add_modal_grab ();
  }

  void MainWindow::disable_command () {
    // hides itself
    command.disable_command ();
    command.remove_modal_grab();
    set_active (current);
    is_command = false;
  }

  void MainWindow::on_command_mode_changed () {
    if (!command.get_search_mode()) {
      disable_command ();
    }
  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    if (is_command) {
      command.command_handle_event (event);
      return true;
    }

    switch (event->keyval) {
      case GDK_KEY_q:
      case GDK_KEY_Q:
        log << info << "mw: quit." << endl;
        astroid->app->remove_window (*this);
        remove_all_modes ();
        close ();
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
        close_page ();
        return true;

      /* search */
      case GDK_KEY_F:
        enable_command (CommandBar::CommandMode::Search, "", NULL);
        return true;

      /* short cut for searching for label */
      case GDK_KEY_L:
        enable_command (CommandBar::CommandMode::Search, "tag:", NULL);
        return true;

      /* command */
      /*
      case GDK_KEY_colon:
        enable_command (CommandBar::CommandMode::Generic, "", NULL);
        return true;
      */

      /* help */
      case GDK_KEY_question:
        {
          HelpMode * h = new HelpMode (this);
          h->show_help ((Mode*) notebook.get_nth_page (notebook.get_current_page()));
          add_mode (h);
          return true;
        }

      /* log window */
      case GDK_KEY_z:
        add_mode (new LogView (this));
        return true;

      /* undo */
      case GDK_KEY_u:
        {
          actions.undo ();
          return true;
        }

      case GDK_KEY_m:
        {
          add_mode (new EditMessage (this));
          return true;
        }

      case GDK_KEY_P:
        {
          astroid->poll->poll ();
          return true;
        }

      case GDK_KEY_O:
        {
          astroid->open_new_window ();
          return true;
        }

    }
    return false;
  }

  ModeHelpInfo * MainWindow::key_help () {
    ModeHelpInfo * m = new ModeHelpInfo ();
    m->toplevel = true;
    m->title = "Main window";

    m->keys = {
      { "m", "Compose new mail" },
      { "P", "Call poll script manually" },
      { "F", "Search" },
      { "L", "Search for tag:" },
      { "u", "Undo last action" },
      { "z", "Show log window" },
      { "?", "Show help" },
      { "O", "Open new main window" },
      /* { ":", "Command" }, */
      { "b,B", "Page through modes" },
      { "x", "Close mode (or window if other windows are open)" },
      { "q,Q", "Quit astroid" },
    };

    return m;
  }

  void MainWindow::add_mode (Mode * m) {
    m = Gtk::manage (m);

    Gtk::Widget * w = m;

    int n = notebook.insert_page ((*w), m->tab_label, current+1);

    notebook.show_all ();

    set_active (n);
  }

  void MainWindow::del_mode (int c) {
    // log << debug << "mw: del mode: " << c << endl;
    if (c >= 0) {
      if (c == 0) {
        set_active (c + 1);
      } else {
        set_active (c - 1);
      }

      notebook.remove_page (c); // this should free the widget (?)
    } else {
      log << warn << "mw: attempt to remove negative page" << endl;
    }
  }

  void MainWindow::remove_all_modes () {
    // used by Astroid::quit to deconstruct all modes before
    // exiting.

    for (int n = notebook.get_n_pages()-1; n >= 0; n--)
      del_mode (n);

  }

  void MainWindow::close_page () {
    /* close current page */
    if (notebook.get_n_pages() > 1) {
      int c = notebook.get_current_page ();
      del_mode (c);
    } else {
      /* if there are more windows, close this one */
      if (astroid->app->get_windows().size () > 1) {
        log << info << "mw: more windows available, closing this one." << endl;
        astroid->app->remove_window (*this);
        remove_all_modes ();
        close ();
      }
    }
  }

  void MainWindow::ungrab_active () {
    if (current >= 0) {
      if (notebook.get_n_pages() > current) {
        //log << debug << "mw: release modal, from: " << current << endl;
        ((Mode*) notebook.get_nth_page (current))->release_modal();
        active = false;
      }
    }
  }

  void MainWindow::on_my_switch_page (Gtk::Widget * /* w */, guint no) {
    grab_active (no);
  }

  void MainWindow::grab_active (int n) {
    //log << debug << "mw: set active: " << n << ", current: " << current << endl;

    ungrab_active ();

    //log << debug << "mw: grab modal to: " << n << endl;

    if (has_focus() || (get_focus() && get_focus()->has_focus())) {
      /* we have focus */
      ((Mode*) notebook.get_nth_page (n))->grab_modal();
    } else {
      log << debug << "mw: does not have focus, will not grab modal." << endl;
    }

    current = n;
    active = true;

    on_update_title ();
  }

  void MainWindow::set_active (int n) {
    //log << debug << "mw: set active: " << n << ", current: " << current << endl;

    if (n >= 0 && n <= notebook.get_n_pages()-1) {

      if (notebook.get_current_page() != n) {
        notebook.set_current_page (n);
      }

      grab_active (n);

    } else {
      // log << debug << "mw: set active: page is out of range: " << n << endl;
      on_update_title ();
    }
  }

  bool MainWindow::on_my_focus_in_event (GdkEventFocus * /* event */) {
    if (active) set_active (current);
    //log << debug << "mw: focus-in: " << id << endl;
    return false;
  }

  bool MainWindow::on_my_focus_out_event (GdkEventFocus * /* event */) {
    //log << debug << "mw: focus-out: " << id << endl;
    if ((current < notebook.get_n_pages ()) && (current >= 0))
      ((Mode*) notebook.get_nth_page (current))->release_modal();
    return false;
  }
}

