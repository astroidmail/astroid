# pragma once

# include <gtkmm.h>
# include <gtkmm/cellrenderer.h>

# include "proto.hh"
# include "db.hh"

using namespace std;

namespace Astroid {
  class ThreadIndexListCellRenderer : public Gtk::CellRenderer {
    public:
      ThreadIndexListCellRenderer ();
      ~ThreadIndexListCellRenderer ();

      Glib::RefPtr<NotmuchThread> thread; /* thread that should be rendered now */
      bool last;
      bool marked;

      /* these tags are displayed otherwisely, so they are not
       * shown explicitly: MUST BE SORTED */
      const vector<ustring> hidden_tags =
              { "attachment", "flagged", "unread"
              };

      int get_height ();

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

      int calculate_height (Gtk::Widget &) const;

      virtual void get_preferred_height_vfunc (
          Gtk::Widget& widget,
          int& minimum_height,
          int& natural_height) const override;

      virtual void get_preferred_height_for_width_vfunc (
          Gtk::Widget& widget,
          int width,
          int& minimum_height,
          int& natural_height) const override;

      virtual void get_preferred_width_vfunc (
          Gtk::Widget& widget,
          int& minimum_width,
          int& natural_width) const override;

      virtual Gtk::SizeRequestMode get_request_mode_vfunc () const override;

    private:
      int content_height;
      int line_spacing = 2;
      int height;
      bool height_set = false;

      int left_icons_size;
      int left_icons_width;
      int left_icons_width_n = 3;
      int left_icons_padding = 1;
      int padding;

      ustring font_desc_string;
      Pango::FontDescription font_description;
      Pango::FontMetrics     font_metrics;

      int date_start;
      int date_len   = 10; // chars
      int date_width;

      int message_count_start;
      int message_count_len = 4; // chars
      int message_count_width;

      int authors_start;
      int authors_len = 20; // chars
      int authors_width;

      int tags_start;
      int tags_width;
      int tags_len = 80; // chars

      int subject_start;

      void render_background (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &background_area,
          Gtk::CellRendererState flags);

      void render_subject (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area,
          Gtk::CellRendererState flags);

      int render_tags (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      int render_date (
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

      refptr<Gdk::Pixbuf> flagged_icon;
      void render_flagged (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      refptr<Gdk::Pixbuf> attachment_icon;
      void render_attachment (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );

      refptr<Gdk::Pixbuf> marked_icon;
      void render_marked (
          const ::Cairo::RefPtr< ::Cairo::Context>&cr,
          Gtk::Widget &widget,
          const Gdk::Rectangle &cell_area );
  };
}

