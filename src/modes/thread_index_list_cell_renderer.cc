/* thread index cell renderer:
 *
 * the heart of showing threads in a fancy / sexy way. renders a
 * nice looking thread entry based on a notmuch thread object -
 * preferably without loading the message file.
 *
 */

# include <iostream>
# include <time.h>

# include <gtkmm/image.h>

# include <notmuch.h>

# include "thread_index_list_cell_renderer.hh"
# include "db.hh"

using namespace std;

namespace Astroid {
  void ThreadIndexListCellRenderer::render_vfunc (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget    &widget,
      const Gdk::Rectangle &background_area,
      const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags)
  {

    //cout << "render..:" << thread->thread_id << endl;

    /*
    auto p = property_cell_background ();
    p.set_value ("#ffffff");
    */

    thread->ensure_valid ();

    render_date (cr, widget, cell_area);
    render_message_count (cr, widget, cell_area);
    render_authors (cr, widget, cell_area);

    tags_width = render_tags (cr, widget, cell_area);
    subject_start = tags_start + tags_width / Pango::SCALE + ((tags_width > 0) ? padding : 0);

    render_subject (cr, widget, cell_area);

    /*
    if (!last)
      render_delimiter (cr, widget, cell_area);
    */

    render_starred (cr, widget, cell_area);

    if (thread->attachment)
      render_attachment (cr, widget, cell_area);

  }

  /* render icons {{{ */
  void ThreadIndexListCellRenderer::render_starred (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {


    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "starred-symbolic",
        left_icons_size,
        Gtk::ICON_LOOKUP_USE_BUILTIN );

    int y = cell_area.get_y() + left_icons_padding;
    int x = cell_area.get_x();

    Gdk::Cairo::set_source_pixbuf (cr, pixbuf, x, y);

    cr->rectangle (x, y, left_icons_size,left_icons_size);
    cr->fill ();
  }

  void ThreadIndexListCellRenderer::render_attachment (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {


    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
        "mail-attachment-symbolic",
        left_icons_size,
        Gtk::ICON_LOOKUP_USE_BUILTIN );

    int y = cell_area.get_y() + left_icons_padding;
    int x = cell_area.get_x() + left_icons_width + left_icons_padding;

    Gdk::Cairo::set_source_pixbuf (cr, pixbuf, x, y);

    cr->rectangle (x, y, left_icons_size, left_icons_size);
    cr->fill ();
  } // }}}

  void ThreadIndexListCellRenderer::render_delimiter ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    cr->set_line_width(0.5);
    Gdk::Color gray;
    gray.set_grey_p (0.1);
    cr->set_source_rgb (gray.get_red_p(), gray.get_green_p(), gray.get_blue_p());

    cr->move_to(cell_area.get_x(), cell_area.get_y() + cell_area.get_height());
    cr->line_to(cell_area.get_x() + cell_area.get_width(), cell_area.get_y() + cell_area.get_height());
    cr->stroke ();

  } // }}}

  void ThreadIndexListCellRenderer::render_subject ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("");

    Pango::FontDescription font_description;
    font_description.set_size(Pango::SCALE * subject_font_size);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();

    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());
    pango_layout->set_markup ("<span color=\"#807d74\">" + thread->subject + "</span>");


    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    cr->move_to (cell_area.get_x() + subject_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  } // }}}

  int ThreadIndexListCellRenderer::render_tags ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("");

    Pango::FontDescription font_description;
    font_description.set_size(Pango::SCALE * subject_font_size);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();

    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    ustring tag_string;

    bool first = true;
    for_each (thread->tags.begin (),
              thread->tags.end (),
              [&](ustring a) {

                if (!first) {
                  tag_string += ",";
                } else {
                  first = false;
                }
                a = a.substr(0,a.find_first_of(" "));
                a = a.substr(0,a.find_first_of("@"));
                tag_string += a;
              });

    tag_string = tag_string.substr (0, tags_max_len);

    pango_layout->set_markup ("<span color=\"#31587a\">" + tag_string + "</span>");

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    cr->move_to (cell_area.get_x() + tags_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

    return w;

  } // }}}

  void ThreadIndexListCellRenderer::render_date ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    time_t newest = thread->newest_date;
    struct tm * timeinfo;
    char buf[80];
    timeinfo = localtime (&newest);
    strftime (buf, 80, "%D %R", timeinfo);


    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (buf);

    Pango::FontDescription font_description;
    font_description.set_size(Pango::SCALE * font_size);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();
    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    /* update subject start */
    //subject_start = date_start + (w / Pango::SCALE) + padding;

    cr->move_to (cell_area.get_x() + date_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  } // }}}

  void ThreadIndexListCellRenderer::render_message_count ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

# define BUFLEN 24
    char buf[BUFLEN];
    snprintf (buf, BUFLEN, "(%d)", thread->total_messages);


    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (buf);

    Pango::FontDescription font_description;
    font_description.set_size(Pango::SCALE * font_size);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();
    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    /* update subject start */
    //subject_start = date_start + (w / Pango::SCALE) + padding;

    cr->move_to (cell_area.get_x() + message_count_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  } // }}}

  void ThreadIndexListCellRenderer::render_authors ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    /* format authors string */
    ustring authors;

    if (thread->authors.size () == 1) {
      /* if only one, show full name */
      authors = thread->authors[0];
    } else {
      /* show first names separated by comma */
      bool first = true;
      for_each (thread->authors.begin (),
                thread->authors.end (),
                [&](ustring a) {

                  if (!first) {
                    authors += ",";
                  } else {
                    first = false;
                  }
                  a = a.substr(0,a.find_first_of(" "));
                  a = a.substr(0,a.find_first_of("@"));
                  authors += a;
                });
    }

    authors = authors.substr (0, authors_max_len);

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (authors);

    Pango::FontDescription font_description;
    font_description.set_size(Pango::SCALE * font_size);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();
    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    /* update subject start */
    //subject_start = date_start + (w / Pango::SCALE) + padding;

    cr->move_to (cell_area.get_x() + authors_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  } // }}}

  /* cellrenderer overloads {{{ */
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
  // }}}

}

