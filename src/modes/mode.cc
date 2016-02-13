# include "mode.hh"
# include "main_window.hh"
# include "keybindings.hh"
# include "log.hh"

namespace Astroid {
  guint32 Mode::mode_event_handled;

  Mode::Mode (MainWindow * mw) :
    Gtk::Box (Gtk::ORIENTATION_VERTICAL)
  {
    set_main_window (mw);
    invincible = false;

    tab_label.set_can_focus (false);

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
    pack_end (*rev_yes_no, false, true, 0);

    yes->signal_clicked().connect (sigc::mem_fun (this, &Mode::on_yes));
    no->signal_clicked().connect (sigc::mem_fun (this, &Mode::on_no));

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
    pack_end (*rev_multi, false, true, 0);

    keys.title = "Mode";
  }

  void Mode::set_main_window (MainWindow *mw) {
    main_window = mw;
  }

  void Mode::set_label (ustring s) {
    if (static_cast<int>(s.size()) > MAX_TAB_LEN)
      s = s.substr(0, MAX_TAB_LEN - 3) + "...";

    tab_label.set_text (s);
    label = s;

    main_window->update_title_dispatcher.emit ();
  }

  void Mode::on_yes () {
    answer_yes_no (true);
  }

  void Mode::on_no () {
    answer_yes_no (false);
  }

  void Mode::close (bool force) {
    /* close current page */
    using std::endl;
    if (main_window->notebook.get_n_pages() > 1) {
      int c = main_window->notebook.get_current_page ();

      if (((Mode*) main_window->notebook.get_nth_page (c))->invincible && !force) {
        log << debug << "mode: mode invincible, not closing." << endl;
      } else {
        main_window->del_mode (c);
      }
    } else {
      /* if there are more windows, close this one */
      if (astroid->app->get_windows().size () > 1) {
        log << debug << "mw: other windows available, closing this one." << endl;
        main_window->quit ();
      }
    }
  }

  void Mode::ask_yes_no (
      ustring question,
      std::function <void (bool)> closure)
  {
    using std::endl;
    log << info << "mode: " << question << endl;

    if (yes_no_waiting || multi_waiting) {
      log << warn << "mode: already waiting for answer to previous question, discarding this one." << endl;
      return;
    }

    /* allow events to be sent to the mode */
    main_window->ungrab_active ();

    yes_no_waiting = true;
    yes_no_closure = closure;

    rev_yes_no->set_reveal_child (true);
    label_yes_no->set_text (question + " [y/n]");
    /* rev_yes_no->add_modal_grab (); */
  }

  void Mode::answer_yes_no (bool yes) {
    using std::endl;
    rev_yes_no->set_reveal_child (false);

    if (yes) {
      log << info << "mode: yes-no: got yes!" << endl;
    } else {
      log << info << "mode: yes-no: got no :/" << endl;
    }

    /* return modal to original mode */
    main_window->grab_active (main_window->current);

    if (yes_no_waiting) {
      if (yes_no_closure != NULL) {
        yes_no_closure (yes);
      }
    }

    yes_no_closure = NULL;
    yes_no_waiting = false;
  }

  bool Mode::multi_key (Keybindings & kb, Key /* k */)
  {
    using std::endl;
    log << info << "mode: starting multi key." << endl;

    if (yes_no_waiting || multi_waiting) {
      log << warn << "mode: already waiting for answer to previous question, discarding this one." << endl;
      return true;
    }

    multi_waiting = true;
    multi_keybindings = kb;

    rev_multi->set_reveal_child (true);
    label_multi->set_text (kb.short_help ());

    return true;
  }

  bool Mode::mode_key_handler (GdkEventKey * event) {
    Widget * wp = get_parent ();
    Mode * parent = dynamic_cast<Mode *> (wp);
    MainWindow * mw = dynamic_cast<MainWindow *> (wp);

    while (mw == NULL) {
      if (parent != NULL) {
        if (parent->mode_key_handler (event)) {
          return true;
        }

        /* only re-run for parent, will be recursively called */
        break;
      }

      wp = wp->get_parent ();
      parent = dynamic_cast<Mode *> (wp);
      mw = dynamic_cast<MainWindow *> (wp);
    }

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

      return res;
    }

    return false;
  }

  bool Mode::on_key_press_event (GdkEventKey *event) {
    /* log << debug << "mode: key: " << typeid (*this).name () << std::endl; */

    /* we need to check all parent widget modes too, before checking for the
     * default keys. */

    if (event->time > mode_event_handled) {
      if (mode_key_handler (event)) return true;
      mode_event_handled = event->time;
    }

    return keys.handle (event);
  }

  ustring Mode::get_label () {
    return label;
  }

  Keybindings * Mode::get_keys () {
    return &keys;
  }
}

