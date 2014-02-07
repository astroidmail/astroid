# pragma once

# include <gtkmm.h>
# include <gtkmm/cellrenderer.h>

# include "proto.hh"
# include "db.hh"

using namespace std;

namespace Gulp {
  class ThreadIndexListCellRenderer : public Gtk::CellRenderer {
    public:
      NotmuchThread thread; /* thread that should be rendered now */

    protected:
      /* best documentation so far from here:
       * https://git.gnome.org/browse/gtkmm/tree/gtk/src/cellrenderer.hg
       */
      virtual void render_vfunc (const ::Cairo::RefPtr< ::Cairo::Context>&,
          Gtk::Widget    &,
          const Gdk::Rectangle &,
          const Gdk::Rectangle &,
          Gtk::CellRendererState) override;

      virtual bool activate_vfunc(
          GdkEvent *,
          Gtk::Widget &,
          const Glib::ustring &,
          const Gdk::Rectangle &,
          const Gdk::Rectangle &,
          Gtk::CellRendererState) override;

  };
}

