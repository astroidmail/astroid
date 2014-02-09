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
      bool last;

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

      virtual void get_preferred_height_vfunc (
          Gtk::Widget& widget,
          int& minimum_height,
          int& natural_height) const override;

    private:
      int content_height = 20;
      int spacing = 2;
      int height = content_height + spacing;
      int left_icons_width = 20;
      int left_icons = 2;
      int padding = 5;
      int font_size = 9;

      int date_start = left_icons*left_icons_width + padding;

      int subject_start; // calculate from previous widget
      int subject_font_size = font_size;

      void render_subject (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_date (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_delimiter (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_starred (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_attachment (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );
  };
}

