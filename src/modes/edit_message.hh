# pragma once

# include "astroid.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  class EditMessage : public Mode {
    public:
      EditMessage ();

      Gtk::Box * box_message;

      void grab_modal () override;
      void release_modal () override;

  };
}

