# pragma once

# include <gtkmm.h>

# include <list>

# include "proto.hh"
# include "keybindings.hh"

using namespace std;

namespace Astroid {
  struct ModeHelpInfo {
    public:
      ModeHelpInfo ();
      ~ModeHelpInfo ();

      ustring title;
      list<pair<ustring, ustring>> keys;

      ModeHelpInfo * parent;
      bool toplevel;
  };

  class Mode : public Gtk::Box {
    public:
      Mode (MainWindow *);
      ~Mode ();
      Gtk::Label tab_label;

      void set_main_window (MainWindow *);
      MainWindow * main_window;

      bool invincible = false;
      void close ();

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
      bool on_key_press_event (GdkEventKey *event) override;
      Keybindings keys;

      void ask_yes_no (ustring, function<void(bool)>);
      bool multi_key (Keybindings &, Key);

      bool mode_key_handler (GdkEventKey *);

      virtual void grab_modal () = 0;
      virtual void release_modal () = 0;
      virtual ModeHelpInfo * key_help ();
      virtual ustring get_label ();
  };
}
