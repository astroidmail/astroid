# pragma once

# include <iostream>

# include <gtkmm.h>

# include "gulp.hh"
# include "proto.hh"
# include "paned_mode.hh"

using namespace std;

namespace Gulp {
  class ThreadView : public Gtk::ScrolledWindow, public PanedChild {
    public:
      void load_thread (ustring);
      void render ();

      ustring thread_id;

      /* paned child */
      virtual void grab_modal () override;
      virtual void release_modal () override;
      virtual Gtk::Widget * get_widget () override;
  };
}

