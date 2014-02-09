/* thread index cell renderer:
 *
 * the heart of showing threads in a fancy / sexy way. renders a
 * nice looking thread entry based on a notmuch thread object -
 * preferably without loading the message file.
 *
 */

# include <iostream>

# include <gtkmm/image.h>

# include <notmuch.h>


# include "thread_index_list_cell_renderer.hh"
# include "db.hh"

using namespace std;

namespace Gulp {
  void ThreadIndexListCellRenderer::render_vfunc (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget    &widget,
      const Gdk::Rectangle &background_area,
      const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags)
  {

    cout << "render..:" << thread.thread_id << endl;

    render_subject (cr, widget, cell_area);
    if (!last)
      render_delimiter (cr, widget, cell_area);
    render_starred (cr, widget, cell_area);
    render_attachment (cr, widget, cell_area);

    cr->stroke ();
  }

  void ThreadIndexListCellRenderer::render_starred (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {


    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "starred-symbolic",
        20,
        Gtk::ICON_LOOKUP_USE_BUILTIN );

    Gdk::Cairo::set_source_pixbuf (cr, pixbuf, cell_area.get_x(), cell_area.get_y());

    cr->rectangle (cell_area.get_x (), cell_area.get_y (), 20, 20);
    cr->fill ();
  }

  void ThreadIndexListCellRenderer::render_attachment (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {


    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "starred-symbolic",
        20,
        Gtk::ICON_LOOKUP_USE_BUILTIN );


    Gdk::Cairo::set_source_pixbuf (cr, pixbuf, cell_area.get_x() + left_icons_width + 2, cell_area.get_y());

    cr->rectangle (cell_area.get_x () + left_icons_width + 2, cell_area.get_y (), 20, 20);
    cr->fill ();
  }

  void ThreadIndexListCellRenderer::render_delimiter (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    cr->set_line_width(0.7);
    Gdk::Color gray;
    gray.set_grey_p (0.5);
    cr->set_source_rgb (gray.get_red_p(), gray.get_green_p(), gray.get_blue_p());

    cr->move_to(cell_area.get_x(), cell_area.get_y() + cell_area.get_height());
    cr->line_to(cell_area.get_x() + cell_area.get_width(), cell_area.get_y() + cell_area.get_height());
    cr->stroke ();

  }

  void ThreadIndexListCellRenderer::render_subject (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ((thread.get_subject()));

    Pango::FontDescription font_description;
    font_description.set_size(Pango::SCALE * subject_font_size);

    if (thread.unread ()) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    pango_layout->set_font_description (font_description);

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    cr->move_to (cell_area.get_x() + subject_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  }


  void ThreadIndexListCellRenderer::get_preferred_height_vfunc (
      Gtk::Widget& widget,
      int& minimum_height,
      int& natural_height) const {

    minimum_height = height;
    natural_height = height;
  }


  bool ThreadIndexListCellRenderer::activate_vfunc(
      GdkEvent* event,
      Gtk::Widget& widget,
      const Glib::ustring& path,
      const Gdk::Rectangle& background_area,
      const Gdk::Rectangle& cell_area,
      Gtk::CellRendererState flags)
  {

    cout << "activate.." << endl;

    return false;
  }

}

