/* thread index cell renderer:
 *
 * the heart of showing threads in a fancy / sexy way. renders a
 * nice looking thread entry based on a notmuch thread object -
 * preferably without loading the message file.
 *
 */

# include <iostream>

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

    //cr->stroke ();
  }

  void ThreadIndexListCellRenderer::render_subject (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ((thread.get_subject()));

    Pango::FontDescription font_description;
    cout << font_description.get_size() << endl;
    font_description.set_size(Pango::SCALE * subject_font_size);
    pango_layout->set_font_description (font_description);

    cr->move_to (cell_area.get_x(), cell_area.get_y());
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

