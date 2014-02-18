# pragma once

# include <vector>
# include <memory>

# include <gtkmm.h>
# include <glibmm.h>

# include "proto.hh"

using namespace std;

namespace Astroid {

  /* aliases for often used types  */
  typedef Glib::ustring ustring;
  # define refptr Glib::RefPtr


  class Astroid {
    public:
      Astroid ();
      int main (int, char**);
      void quit ();

      Glib::RefPtr<Gtk::Application> app;
      Db *db;

      /* list of main windows */
      vector<MainWindow*>  main_windows;


  };

  /* globally available instance of our main Astroid-class */
  extern Astroid * astroid;
}

