# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>
# include <gtkmm/notebook.h>

# include "astroid.hh"
# include "build_config.hh"
# include "poll.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index/thread_index.hh"
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

    /* register keys {{{ */
    keys.register_key ("q", { Key ("Q") },
        "main_window.quit_ask",
        "Quit astroid",
        [&] (Key k) {
          if (k.key == GDK_KEY_q) {
            if (astroid->app->get_windows().size () > 1) {
              /* other windows, just close this one */
              quit ();
            } else {
              Mode * m = (Mode *) notebook.get_children()[notebook.get_current_page ()];
              m->ask_yes_no ("Really quit?", [&](bool yes){ if (yes) quit (); });
            }
          } else if (k.key == GDK_KEY_Q) {
            quit ();
          }

          return true;
        });

    keys.register_key ("b", "main_window.next_page",
        "Next page",
        [&] (Key)
        {
          if (notebook.get_current_page () == (notebook.get_n_pages () - 1))
            set_active (0);
          else
            set_active (notebook.get_current_page() + 1);

          return true;
        });

    keys.register_key ("B", "main_window.previous_page",
        "Previous page",
        [&] (Key) {
          if (notebook.get_current_page() == 0)
            set_active (notebook.get_n_pages()-1);
          else
            set_active (notebook.get_current_page() - 1);

          return true;
        });

    keys.register_key (Key(false, true, (guint) GDK_KEY_1),
        { Key (false, true, (guint) GDK_KEY_2),
          Key (false, true, (guint) GDK_KEY_3),
          Key (false, true, (guint) GDK_KEY_4),
          Key (false, true, (guint) GDK_KEY_5),
          Key (false, true, (guint) GDK_KEY_6),
          Key (false, true, (guint) GDK_KEY_7),
          Key (false, true, (guint) GDK_KEY_8),
          Key (false, true, (guint) GDK_KEY_9),
          Key (false, true, (guint) GDK_KEY_0) },
        "main_window.jump_to_page",
        "Jump to page",
        [&] (Key k) {
          int pg = k.key - GDK_KEY_0;

          if (pg == 0) {
            pg = notebook.get_n_pages () - 1;
          } else {
            pg--;
          }

          log << debug << "mw: swapping to page: " << pg << endl;

          if (notebook.get_current_page () != pg) {
            set_active (pg);
          }


          return true;
        });

    keys.register_key ("x", "main_window.close_page",
        "Close mode (or window if other windows are open)",
        [&] (Key) {
          close_page ();
          return true;
        });

    keys.register_key ("F", "main_window.search",
        "Search",
        [&] (Key) {
          enable_command (CommandBar::CommandMode::Search, "", NULL);
          return true;
        });

    keys.register_key ("L", "main_window.search_tag",
        "Search for tag:",
        [&] (Key) {
          enable_command (CommandBar::CommandMode::Search, "tag:", NULL);
          return true;
        });


    keys.register_key (Key (GDK_KEY_question), "main_window.show_help",
        "Show help",
        [&] (Key) {
          HelpMode * h = new HelpMode (this);
          h->show_help ((Mode*) notebook.get_nth_page (notebook.get_current_page()));
          add_mode (h);
          return true;
        });

    keys.register_key ("z", "main_window.show_log",
        "Show log window",
        [&] (Key) {
          add_mode (new LogView (this));
          return true;
        });

    keys.register_key ("u", "main_window.undo",
        "Undo last action",
        [&] (Key) {
          actions.undo ();
          return true;
        });

    keys.register_key ("m", "main_window.new_mail",
        "Compose new mail",
        [&] (Key) {
          add_mode (new EditMessage (this));
          return true;
        });

    keys.register_key ("P", "main_window.poll",
        "Start manual poll",
        [&] (Key) {
          astroid->poll->poll ();
          return true;
        });

    keys.register_key ("C-p", "main_window.toggle_auto_poll",
        "Toggle auto poll",
        [&] (Key) {
          astroid->poll->toggle_auto_poll ();
          return true;
        });

    keys.register_key ("O", "main_window.open_new_window",
        "Open new main window",
        [&] (Key) {
          astroid->open_new_window ();
          return true;
        });

    // }}}
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

  void MainWindow::quit () {
    log << info << "mw: quit." << endl;
    astroid->app->remove_window (*this);
    remove_all_modes ();
    close ();
  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    if (is_command) {
      command.command_handle_event (event);
      return true;
    }

    return keys.handle (event);
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

  void MainWindow::close_page (Mode * m) {
    if (notebook.get_n_pages() > 1) {
      if (!m->invincible) {
        for (int c = 0; c < notebook.get_n_pages (); c++) {
          if (m == (Mode*) notebook.get_nth_page (c)) {
            del_mode (c);
            break;
          }
        }
      }
    } else {
      /* if there are more windows, close this one */
      if (astroid->app->get_windows().size () > 1) {
        log << debug << "mw: other windows available, closing this one." << endl;
        quit ();
      }
    }
  }

  void MainWindow::close_page () {
    /* close current page */
    if (notebook.get_n_pages() > 1) {
      int c = notebook.get_current_page ();

      if (!((Mode*) notebook.get_nth_page (c))->invincible) {
        del_mode (c);
      } else {
        log << debug << "mw: mode invincible, not closing." << endl;
      }
    } else {
      /* if there are more windows, close this one */
      if (astroid->app->get_windows().size () > 1) {
        log << debug << "mw: other windows available, closing this one." << endl;
        quit ();
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

