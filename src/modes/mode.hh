# pragma once

# include <gtkmm.h>
# include <atomic>

# include <list>

# include "proto.hh"
# include "keybindings.hh"

using namespace std;

namespace Astroid {
  class Mode : public Gtk::Box {
    public:
      Mode (MainWindow *);
      Gtk::Label tab_label;

      void set_main_window (MainWindow *);
      MainWindow * main_window;

      atomic<bool> invincible;
      virtual void close (bool = false);

    private:
      ustring label;

      /* yes no questions */
      Gtk::Revealer * rev_yes_no;
      Gtk::Label    * label_yes_no;

      bool yes_no_waiting = false;
      function <void (bool)> yes_no_closure = NULL;
      void answer_yes_no (bool);
      void on_yes ();
      void on_no ();

      /* multi key */
      Gtk::Revealer * rev_multi;
      Gtk::Label    * label_multi;

      bool multi_waiting  = false;
      Keybindings multi_keybindings;

    protected:
      const int MAX_TAB_LEN = 35;
      void set_label (ustring);

    public:
      Keybindings keys;
      bool on_key_press_event (GdkEventKey *event) override;
      virtual Keybindings * get_keys ();

      void ask_yes_no (ustring, function<void(bool)>);
      bool multi_key (Keybindings &, Key);

      bool mode_key_handler (GdkEventKey *);

      virtual void grab_modal () = 0;
      virtual void release_modal () = 0;
      virtual ustring get_label ();
  };
}
