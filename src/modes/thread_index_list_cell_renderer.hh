# pragma once

# include <gtkmm.h>
# include <gtkmm/cellrenderer.h>

# include "proto.hh"
# include "db.hh"

using namespace std;

namespace Astroid {
  class ThreadIndexListCellRenderer : public Gtk::CellRenderer {
    public:
      Glib::RefPtr<NotmuchThread> thread; /* thread that should be rendered now */
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
      int line_spacing = 2;
      int height = content_height + line_spacing;
      int left_icons_size = 15;
      int left_icons_width  = 15;
      int left_icons_width_n = 2;
      int left_icons_padding = 2;
      int padding = 5;
      int font_size = 9;

      int date_start = left_icons_width_n * left_icons_width +
        (left_icons_width_n-1) * left_icons_padding + padding;
      int date_width = 100;

      int message_count_start  = date_start + date_width;
      int message_count_width = 30;

      int authors_start = message_count_start + message_count_width;
      int authors_width = 150;
      int authors_max_len = 20;

      int tags_start = authors_start + authors_width;
      int tags_width = 150;
      int tags_max_len = 80;

      int subject_start = tags_start + tags_width;
      int subject_font_size = font_size;

      void render_background (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &background_area,
          Gtk::CellRendererState flags);

      void render_subject (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      int render_tags (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_date (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_message_count (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      void render_authors (
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

