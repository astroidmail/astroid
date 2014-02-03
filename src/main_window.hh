# pragma once

# include <gtkmm.h>
# include <gtkmm/window.h>

# include "proto.hh"

using namespace std;

namespace Gulp {
  class Notebook : public Gtk::Notebook {

  };

  class MainWindow : public Gtk::Window {
    public:
      MainWindow ();
      ~MainWindow ();

      Notebook notebook;

      vector<Mode*> modes; 

      void add_mode (Mode *);
      void del_mode (Mode *);

  };

}

