# pragma once

# include <gtkmm.h>

# include "proto.hh"

using namespace std;

namespace Astroid {
  class Mode : public Gtk::Box {
    public:
      Mode (bool interactive = false);
      ~Mode ();
      Gtk::Widget * tab_widget;

    private:
      bool interactive;

      /* yes no questions */
      Gtk::Revealer * rev_yes_no;
      Gtk::Label    * label_yes_no;

      bool yes_no_waiting = false;
      function <void (bool)> yes_no_closure = NULL;
      void answer_yes_no (bool);

    protected:
      void ask_yes_no (ustring, function<void(bool)>);
      bool mode_key_handler (GdkEventKey *);

    public:
      virtual void grab_modal () = 0;
      virtual void release_modal () = 0;
  };
}
