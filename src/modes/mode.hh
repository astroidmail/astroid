# pragma once

# include <gtkmm.h>
# include <atomic>
# include <functional>

# include "proto.hh"
# include "keybindings.hh"

namespace Astroid {
  class Mode : public Gtk::Box {
    public:
      Mode (MainWindow *);
      Gtk::Label tab_label;

      void set_main_window (MainWindow *);
      MainWindow * main_window;

      std::atomic<bool> invincible;
      virtual void close (bool = false);
      virtual void pre_close ();

    private:
      ustring label;


    protected:
      const int MAX_TAB_LEN = 35;
      void set_label (ustring);

    public:
      Keybindings keys;
      bool on_key_press_event (GdkEventKey *event) override;
      virtual Keybindings * get_keys ();

      virtual void grab_modal () = 0;
      virtual void release_modal () = 0;
      virtual ustring get_label ();

      /* wrappers to MainWindow */
      void ask_yes_no (ustring, std::function<void(bool)>);
      bool multi_key (Keybindings &, Key);
  };
}
