# pragma once

# include <string>

# include <gtkmm.h>
# include <gtkmm/box.h>
# include <gtkmm/widget.h>

using namespace std;

namespace Gulp {
  class Mode : public Gtk::Box {
    public:
      Mode ();
      ~Mode ();
      Gtk::Widget * tab_widget;

      virtual void grab_modal () = 0;
  };
}
