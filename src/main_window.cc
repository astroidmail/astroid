# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>
# include <gtkmm/notebook.h>

# include "astroid.hh"
# include "poll.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index/thread_index.hh"
# include "modes/saved_searches.hh"
# include "modes/help_mode.hh"
# include "modes/edit_message.hh"
# include "modes/log_view.hh"
# include "command_bar.hh"
# include "actions/action.hh"
# include "actions/action_manager.hh"

using namespace std;

namespace Astroid {
  atomic<uint> MainWindow::nextid (0);
  int          Notebook::icon_size = 42;

  Notebook::Notebook () {
    set_scrollable (true);

    set_action_widget (&icons, Gtk::PACK_END);
    icons.show_all ();
    icon_size = icons.get_height ();

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

    actions = astroid->actions;

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

    /* set up yes-no asker */
    rev_yes_no = Gtk::manage (new Gtk::Revealer ());
    rev_yes_no->set_transition_type (Gtk::REVEALER_TRANSITION_TYPE_SLIDE_UP);

    Gtk::Box * rh = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

    label_yes_no = Gtk::manage (new Gtk::Label ());
    rh->pack_start (*label_yes_no, true, true, 5);
    label_yes_no->set_halign (Gtk::ALIGN_START);

    /* buttons */
    Gtk::Button * yes = Gtk::manage (new Gtk::Button("_Yes"));
    Gtk::Button * no  = Gtk::manage (new Gtk::Button("_No"));

    yes->set_use_underline (true);
    no->set_use_underline (true);

    rh->pack_start (*yes, false, true, 5);
    rh->pack_start (*no, false, true, 5);

    rev_yes_no->set_margin_top (0);
    rh->set_margin_bottom (5);

    rev_yes_no->add (*rh);
    rev_yes_no->set_reveal_child (false);
    vbox.pack_end (*rev_yes_no, false, true, 0);

    yes->signal_clicked().connect (sigc::mem_fun (this, &MainWindow::on_yes));
    no->signal_clicked().connect (sigc::mem_fun (this, &MainWindow::on_no));

    /* multi key handler */
    rev_multi = Gtk::manage (new Gtk::Revealer ());
    rev_multi->set_transition_type (Gtk::REVEALER_TRANSITION_TYPE_SLIDE_UP);

    Gtk::Box * rh_ = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

    label_multi = Gtk::manage (new Gtk::Label ());
    rh_->pack_start (*label_multi, true, true, 5);
    label_multi->set_halign (Gtk::ALIGN_START);

    rev_multi->set_margin_top (0);
    rh->set_margin_bottom (5);

    rev_multi->add (*rh_);
    rev_multi->set_reveal_child (false);
    vbox.pack_end (*rev_multi, false, true, 0);


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
    keys.title = "MainWindow";

    keys.register_key ("q",
        "main_window.quit_ask",
        "Quit astroid",
        [&] (Key) {
          if (astroid->app->get_windows().size () > 1) {
            /* other windows, just close this one */
            quit ();
          } else {
            ask_yes_no ("Really quit?", [&](bool yes){ if (yes) quit (); });
          }
          return true;
        });

    keys.register_key ("Q",
        "main_window.quit",
        "Quit astroid (without asking)",
        [&] (Key) {
          quit ();
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

    keys.register_key ("M-1",
        "main_window.jump_to_page_1",
        "Jump to page 1",
        bind (&MainWindow::jump_to_page, this, _1, 1));

    keys.register_key ("M-2",
        "main_window.jump_to_page_2",
        "Jump to page 2",
        bind (&MainWindow::jump_to_page, this, _1, 2));

    keys.register_key ("M-3",
        "main_window.jump_to_page_3",
        "Jump to page 3",
        bind (&MainWindow::jump_to_page, this, _1, 3));

    keys.register_key ("M-4",
        "main_window.jump_to_page_4",
        "Jump to page 4",
        bind (&MainWindow::jump_to_page, this, _1, 4));

    keys.register_key ("M-5",
        "main_window.jump_to_page_5",
        "Jump to page 5",
        bind (&MainWindow::jump_to_page, this, _1, 5));

    keys.register_key ("M-6",
        "main_window.jump_to_page_6",
        "Jump to page 6",
        bind (&MainWindow::jump_to_page, this, _1, 6));

    keys.register_key ("M-7",
        "main_window.jump_to_page_7",
        "Jump to page 7",
        bind (&MainWindow::jump_to_page, this, _1, 7));

    keys.register_key ("M-8",
        "main_window.jump_to_page_8",
        "Jump to page 8",
        bind (&MainWindow::jump_to_page, this, _1, 8));

    keys.register_key ("M-9",
        "main_window.jump_to_page_9",
        "Jump to page 9",
        bind (&MainWindow::jump_to_page, this, _1, 9));

    keys.register_key ("M-0",
        "main_window.jump_to_page_0",
        "Jump to page 0",
        bind (&MainWindow::jump_to_page, this, _1, 0));

    keys.register_key ("x", "main_window.close_page",
        "Close mode (or window if other windows are open)",
        [&] (Key) {
          close_page ();
          return true;
        });

    keys.register_key ("X", "main_window.close_page_force",
        "Force close mode (or window if other windows are open)",
        [&] (Key) {
          close_page (true);
          return true;
        });

    keys.register_key ("F", "main_window.search",
        "Search",
        [&] (Key) {
          enable_command (CommandBar::CommandMode::Search, "", NULL);
          return true;
        });

    keys.register_key ("C-f", "main_window.show_saved_searches",
        "Show saved searches",
        [&] (Key) {
          add_mode (new SavedSearches (this));
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
          actions->undo ();
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

    keys.register_key ("M-p", "main_window.toggle_auto_poll",
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

  bool MainWindow::jump_to_page (Key, int pg) {
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
  }

  void MainWindow::set_title (ustring t) {

    ustring tt = ustring::compose( "Astroid (%1)", Astroid::version);

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

  void MainWindow::enable_command (CommandBar::CommandMode m, ustring title, ustring cmd, function<void(ustring)> f) {
    ungrab_active ();
    command.enable_command (m, title, cmd, f);
    is_command = true;
    command.add_modal_grab ();
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
    in_quit = true;

    astroid->app->remove_window (*this);
    remove_all_modes ();
    close ();
  }

  void MainWindow::on_yes () {
    answer_yes_no (true);
  }

  void MainWindow::on_no () {
    answer_yes_no (false);
  }


  bool MainWindow::mode_key_handler (GdkEventKey * event) {
    if (yes_no_waiting) {
      switch (event->keyval) {
        case GDK_KEY_Y:
        case GDK_KEY_y:
          answer_yes_no (true);
          return true;

        case GDK_KEY_Escape:
        case GDK_KEY_N:
        case GDK_KEY_n:
          answer_yes_no (false);
          return true;
      }

      /* swallow all other keys */
      return true;

    } else if (multi_waiting) {
      bool res = false;

      switch (event->keyval) {
        case GDK_KEY_Escape:
          {
            res = true;
          }
          break;

        default:
          {
            res = multi_keybindings.handle (event);
          }
          break;
      }

      /* close rev */
      multi_waiting = !res;

      if (res) {
        rev_multi->set_reveal_child (false);
        multi_keybindings.clear ();
      }

      return true; // swallow all keys
    }

    return false;
  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    if (mode_key_handler (event)) return true;

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

  void MainWindow::ask_yes_no (
      ustring question,
      std::function <void (bool)> closure)
  {
    using std::endl;
    log << info << "mw: " << question << endl;

    if (yes_no_waiting || multi_waiting) {
      log << warn << "mw: already waiting for answer to previous question, discarding this one." << endl;
      return;
    }

    yes_no_waiting = true;
    yes_no_closure = closure;

    rev_yes_no->set_reveal_child (true);
    label_yes_no->set_text (question + " [y/n]");
  }

  void MainWindow::answer_yes_no (bool yes) {
    using std::endl;
    rev_yes_no->set_reveal_child (false);

    if (yes) {
      log << info << "mw: yes-no: got yes!" << endl;
    } else {
      log << info << "mw: yes-no: got no :/" << endl;
    }

    if (yes_no_waiting) {
      if (yes_no_closure != NULL) {
        yes_no_closure (yes);
      }
    }

    yes_no_closure = NULL;
    yes_no_waiting = false;
  }

  bool MainWindow::multi_key (Keybindings & kb, Key /* k */)
  {
    using std::endl;
    log << info << "mw: starting multi key." << endl;

    if (yes_no_waiting || multi_waiting) {
      log << warn << "mw: already waiting for answer to previous question, discarding this one." << endl;
      return true;
    }

    multi_waiting = true;
    multi_keybindings = kb;

    rev_multi->set_reveal_child (true);
    label_multi->set_text (kb.short_help ());

    return true;
  }

  void MainWindow::del_mode (int c) {
    // log << debug << "mw: del mode: " << c << endl;
    if (notebook.get_n_pages() > 1) {
      if (c >= 0) {
        if (c == 0) {
          set_active (c + 1);
        } else {
          set_active (c - 1);
        }

        ((Mode*) notebook.get_nth_page (c))->pre_close ();
        notebook.remove_page (c); // this should free the widget (?)
      } else {
        log << warn << "mw: attempt to remove negative page" << endl;
      }
    } else {
      if (!in_quit && astroid->app->get_windows().size () > 1) {
        log << debug << "mw: other windows available, closing this one." << endl;
        quit ();
      }
    }
  }

  void MainWindow::remove_all_modes () {
    // used by Astroid::quit to deconstruct all modes before
    // exiting.

    for (int n = notebook.get_n_pages(); n > 0; n--) {
      close_page (true);
    }

  }

  void MainWindow::close_page (Mode * m, bool force) {
    m->close (force);
  }

  void MainWindow::close_page (bool force) {
    int c = notebook.get_current_page ();

    ((Mode*) notebook.get_nth_page (c))->close (force);
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

