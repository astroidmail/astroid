# pragma once

# include <gtkmm.h>
# include <gtkmm/cellrenderer.h>

# include "../proto.hh"
# include "../db.hh"

using namespace std;

namespace Gulp {
  class ThreadIndexListCellRenderer : public Gtk::CellRenderer {
    public:
      NotmuchThread thread;

  };
}

