# pragma once

# include <iostream>

# include <gtkmm.h>

# include "gulp.hh"
# include "proto.hh"
# include "mode.hh"

using namespace std;

namespace Gulp {
  class ThreadView : public Mode {
    public:
      ThreadView ();
      void load_thread (ustring);
      void render ();

      ustring thread_id;

      Gtk::ScrolledWindow scroll;

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

