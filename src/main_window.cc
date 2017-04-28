# include <iostream>

# include <gtkmm.h>
# include <gtkmm/widget.h>
# include <gtkmm/notebook.h>

# ifndef DISABLE_VTE
# include <vte/vte.h>
# endif

# include <boost/filesystem.hpp>

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
# include "utils/resource.hh"

using namespace std;
namespace bfs = boost::filesystem;

# ifndef DISABLE_VTE
extern "C" {
  void mw_on_terminal_child_exit (VteTerminal * t, gint a, gpointer mw) {
    ((Astroid::MainWindow *) mw)->on_terminal_child_exit (t, a);
  }

  void mw_on_terminal_commit (VteTerminal * t, gchar ** tx, guint sz, gpointer mw) {
    ((Astroid::MainWindow *) mw)->on_terminal_commit (t, tx, sz);
  }

# if VTE_CHECK_VERSION(0,48,0)
  void mw_on_terminal_spawn_callback (VteTerminal * t, GPid pid, GError * err, gpointer mw)
  {
    ((Astroid::MainWindow *) mw)->on_terminal_spawn_callback (t, pid, err);
  }
# endif
}
# endif

namespace Astroid {
  atomic<uint> MainWindow::nextid (0);
  int          Notebook::icon_size = 42;

  Notebook::Notebook () {
    set_scrollable (true);

    set_action_widget (&icons, Gtk::PACK_END);
    icons.show_all ();

    astroid->poll->signal_poll_state ().connect (
        sigc::mem_fun (this, &Notebook::poll_state_changed));
    signal_size_allocate ().connect (
        sigc::mem_fun (this, &Notebook::on_my_size_allocate));

    poll_state_changed (astroid->poll->get_poll_state());
  }

  void Notebook::on_my_size_allocate (Gtk::Allocation &) {
    icon_size = icons.get_height ();
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

    LOG (debug) << "mw: init, id: " << id;

    set_title ("");
    set_default_size (1200, 800);

    path icon = Resource (false, "ui/icons/icon_color.png").get_path ();
    try {
      refptr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file (icon.c_str (), 42, 42, true);
      set_icon (pixbuf);
    } catch (Gdk::PixbufError &e) {
      LOG (error) << "mw: could not set icon: " << e.what ();
    }

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

    /* terminal */
# ifndef DISABLE_VTE
    rev_terminal = Gtk::manage (new Gtk::Revealer ());
    rev_terminal->set_transition_type (Gtk::REVEALER_TRANSITION_TYPE_SLIDE_UP);
    rev_terminal->set_reveal_child (false);
    vbox.pack_end (*rev_terminal, false, true, 0);

    terminal_cwd = bfs::current_path ();
# endif

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
          if (astroid->get_windows().size () > 1) {
            /* other windows, just close this one */
            quit ();
          } else {
            LOG (debug) << "really quit?: " << id;
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

    keys.register_key ("l", { "b" },
        "main_window.next_page",
        "Next page",
        [&] (Key)
        {
          if (notebook.get_current_page () == (notebook.get_n_pages () - 1))
            set_active (0);
          else
            set_active (notebook.get_current_page() + 1);

          return true;
        });

    keys.register_key ("h", { "B" },
        "main_window.previous_page",
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

    keys.register_key ("C-w", "main_window.close_page",
        "Close mode (or window if other windows are open)",
        [&] (Key) {
          close_page ();
          return true;
        });

    keys.register_key ("C-W", "main_window.close_page_force",
        "Force close mode (or window if other windows are open)",
        [&] (Key) {
          close_page (true);
          return true;
        });

    keys.register_key ("o",
        "main_window.search",
        "Search",
        [&] (Key) {
          enable_command (CommandBar::CommandMode::Search, "", NULL);
          return true;
        });

    keys.register_key ("M-s", "main_window.show_saved_searches",
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

    keys.register_key ("C-c", "main_window.cancel_poll",
        "Cancel ongoing poll",
        [&] (Key) {
          astroid->poll->cancel_poll ();
          return true;
        });

    keys.register_key ("C-o", "main_window.open_new_window",
        "Open new main window",
        [&] (Key) {
          astroid->open_new_window ();
          return true;
        });

# ifndef DISABLE_VTE
    keys.register_key ("|", "main_window.open_terminal",
        "Open terminal",
        [&] (Key) {
          enable_terminal ();
          return true;
        });
# endif

    // }}}
  }

  bool MainWindow::jump_to_page (Key, int pg) {
    if (pg == 0) {
      pg = notebook.get_n_pages () - 1;
    } else {
      pg--;
    }

    LOG (debug) << "mw: swapping to page: " << pg;

    if (notebook.get_current_page () != pg) {
      set_active (pg);
    }

    return true;
  }

  bool MainWindow::is_current (Mode * m) {
    int i = notebook.get_current_page ();
    return (m == ((Mode *) notebook.get_nth_page (i)));
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
    active_mode = Command;
    command.add_modal_grab ();
  }

  void MainWindow::enable_command (CommandBar::CommandMode m, ustring cmd, function<void(ustring)> f) {
    ungrab_active ();
    command.enable_command (m, cmd, f);
    active_mode = Command;
    command.add_modal_grab ();
  }

  void MainWindow::disable_command () {
    // hides itself
    command.remove_modal_grab();
    set_active (current);
    active_mode = Window;
  }

  void MainWindow::on_command_mode_changed () {
    if (!command.get_search_mode()) {
      disable_command ();
    }
  }

  /* Terminal {{{ */
# ifndef DISABLE_VTE
  void MainWindow::enable_terminal () {
    rev_terminal->set_reveal_child (true);
    ungrab_active ();
    active_mode = Terminal;

    vte_term = vte_terminal_new ();
    gtk_container_add (GTK_CONTAINER(rev_terminal->gobj ()), vte_term);
    rev_terminal->show_all ();

    vte_terminal_set_size (VTE_TERMINAL (vte_term), 1, 10);

    /* start shell */
    char * shell = vte_get_user_shell ();

    char * args[2] = { shell, NULL };
    char * envs[1] = { NULL };

    LOG (info) << "mw: starting terminal..: " << shell;

    if (!bfs::exists (terminal_cwd)) {
      terminal_cwd = bfs::current_path ();
    }

# if VTE_CHECK_VERSION(0,48,0)
    vte_terminal_spawn_async (VTE_TERMINAL(vte_term),
        VTE_PTY_DEFAULT,
        terminal_cwd.c_str(),
        args,
        envs,
        G_SPAWN_DEFAULT,
        NULL,
        NULL,
        NULL,
        -1,
        NULL,
        mw_on_terminal_spawn_callback,
        this);
# else
    GError * err = NULL;
    vte_terminal_spawn_sync (VTE_TERMINAL(vte_term),
        VTE_PTY_DEFAULT,
        terminal_cwd.c_str(),
        args,
        envs,
        G_SPAWN_DEFAULT,
        NULL,
        NULL,
        &terminal_pid,
        NULL,
        (err = NULL, &err));

    on_terminal_spawn_callback (VTE_TERMINAL(vte_term), terminal_pid, err);

# endif

    gtk_widget_grab_focus (vte_term);
    gtk_grab_add (vte_term);
  }

  void MainWindow::on_terminal_spawn_callback (VteTerminal * vte_term, GPid pid, GError * err)
  {
    if (err) {
      LOG (error) << "mw: terminal: " << err->message;
      disable_terminal ();
    } else {
      terminal_pid = pid;

      LOG (debug) << "mw: terminal started: " << terminal_pid;
      g_signal_connect (vte_term, "child-exited",
          G_CALLBACK (mw_on_terminal_child_exit),
          (gpointer) this);

      g_signal_connect (vte_term, "commit",
          G_CALLBACK (mw_on_terminal_commit),
          (gpointer) this);

    }
  }

  void MainWindow::disable_terminal () {
    LOG (info) << "mw: disabling terminal..";
    rev_terminal->set_reveal_child (false);
    set_active (current);
    active_mode = Window;
    gtk_grab_remove (vte_term);

    gtk_widget_destroy (vte_term);
  }

  void MainWindow::on_terminal_child_exit (VteTerminal *, gint) {
    LOG (info) << "mw: terminal exited (cwd: " << terminal_cwd.c_str () << ")";
    disable_terminal ();
  }

  void MainWindow::on_terminal_commit (VteTerminal *, gchar **, guint) {
    bfs::path pth = bfs::path(ustring::compose ("/proc/%1/cwd", terminal_pid).c_str ());

    if (bfs::exists (pth)) {
      terminal_cwd = bfs::canonical (pth);
    }
  }
# endif

  // }}}

  void MainWindow::quit () {
    LOG (info) << "mw: quit: " << id;
    in_quit = true;

    /* focus out */
    ungrab_active ();

    /* remove all modes */
    /* for (int n = notebook.get_n_pages(); n > 0; n--) { */
    /*   close_page (true); */
    /* } */

    close (); // Gtk::Window::close ()
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

# ifndef DISABLE_VTE
    } else if (active_mode == Terminal) {
      return true;
    }
# else
    }
# endif

    return false;
  }

  bool MainWindow::on_key_press (GdkEventKey * event) {
    if (mode_key_handler (event)) return true;

    if (active_mode == Command) {
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
    LOG (info) << "mw: " << question;

    if (yes_no_waiting || multi_waiting) {
      LOG (warn) << "mw: already waiting for answer to previous question, discarding this one.";
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
      LOG (info) << "mw: yes-no: got yes!";
    } else {
      LOG (info) << "mw: yes-no: got no :/";
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
    LOG (info) << "mw: starting multi key.";

    if (yes_no_waiting || multi_waiting) {
      LOG (warn) << "mw: already waiting for answer to previous question, discarding this one.";
      return true;
    }

    multi_waiting = true;
    multi_keybindings = kb;

    rev_multi->set_reveal_child (true);
    label_multi->set_markup (kb.short_help ());

    return true;
  }

  void MainWindow::del_mode (int c) {
    // LOG (debug) << "mw: del mode: " << c;
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
        LOG (warn) << "mw: attempt to remove negative page";
      }
    } else {
      if (!in_quit && astroid->get_windows().size () > 1) {
        LOG (debug) << "mw: other windows available, closing this one.";
        quit ();
      }
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
        //LOG (debug) << "mw: release modal, from: " << current;
        ((Mode*) notebook.get_nth_page (current))->release_modal();
        active = false;
      }
    }
  }

  void MainWindow::on_my_switch_page (Gtk::Widget * /* w */, guint no) {
    grab_active (no);
  }

  void MainWindow::grab_active (int n) {
    //LOG (debug) << "mw: set active: " << n << ", current: " << current;

    ungrab_active ();

    //LOG (debug) << "mw: grab modal to: " << n;

    if (has_focus() || (get_focus() && get_focus()->has_focus())) {
      /* we have focus */
      ((Mode*) notebook.get_nth_page (n))->grab_modal();
    } else {
      LOG (debug) << "mw: does not have focus, will not grab modal.";
    }

    current = n;
    active = true;

    on_update_title ();
  }

  void MainWindow::set_active (int n) {
    //LOG (debug) << "mw: set active: " << n << ", current: " << current;

    if (n >= 0 && n <= notebook.get_n_pages()-1) {

      if (notebook.get_current_page() != n) {
        notebook.set_current_page (n);
      }

      grab_active (n);

    } else {
      // LOG (debug) << "mw: set active: page is out of range: " << n;
      on_update_title ();
    }
  }

  bool MainWindow::on_my_focus_in_event (GdkEventFocus * /* event */) {
    if (!in_quit && active) set_active (current);
    LOG (debug) << "mw: focus-in: " << id << " active: " << active << ", in_quit: " << in_quit;
    return false;
  }

  bool MainWindow::on_my_focus_out_event (GdkEventFocus * /* event */) {
    //LOG (debug) << "mw: focus-out: " << id;
    if ((current < notebook.get_n_pages ()) && (current >= 0))
      ((Mode*) notebook.get_nth_page (current))->release_modal();
    return false;
  }
}

