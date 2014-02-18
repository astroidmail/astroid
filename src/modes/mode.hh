# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/widget.h>

using namespace std;

namespace Astroid {
  class Mode : public Gtk::Box {
    public:
      Mode ();
      ~Mode ();
      Gtk::Widget * tab_widget;

      virtual void grab_modal () = 0;
      virtual void release_modal () = 0;
  };
}
