# include "mode.hh"
# include "main_window.hh"
# include "keybindings.hh"

namespace Astroid {
  Mode::Mode (MainWindow * mw) :
    Gtk::Box (Gtk::ORIENTATION_VERTICAL)
  {
    set_main_window (mw);
    invincible = false;

    tab_label.set_can_focus (false);

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

  void Mode::pre_close () {
    /* allow sub-modes to clean up anything when we are sure that the
     * mode will be closed */
  }

  void Mode::close (bool force) {
    /* close current page */
    using std::endl;
    int c = main_window->notebook.get_current_page ();

    if (((Mode*) main_window->notebook.get_nth_page (c))->invincible && !force) {
      LOG (debug) << "mode: mode invincible, not closing.";
    } else {
      main_window->del_mode (c);
    }
  }

  void Mode::ask_yes_no (
      ustring question,
      std::function <void (bool)> closure)
  {
    return main_window->ask_yes_no (question, closure);
  }

  bool Mode::multi_key (Keybindings & kb, Key k)
  {
    return main_window->multi_key (kb, k);
  }

  bool Mode::on_key_press_event (GdkEventKey *event) {
    /* check if there are any dialogs (question-bars) open on the main_window */

    if (main_window->mode_key_handler (event)) return true;

    if (get_keys ()->handle (event))  return true;

    return false;
  }

  ustring Mode::get_label () {
    return label;
  }

  Keybindings * Mode::get_keys () {
    return &keys;
  }
}

