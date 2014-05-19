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
# include "utils/utils.hh"

using namespace std;

namespace Astroid {

  ThreadIndexListCellRenderer::ThreadIndexListCellRenderer () {

  }


  void ThreadIndexListCellRenderer::render_vfunc (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget    &widget,
      const Gdk::Rectangle &background_area,
      const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags)
  {
    // calculate text width, we don't need to do this every time,
    // but we need access to the context.
    refptr<Pango::Context> pango_cr = widget.create_pango_context ();
    font_description.set_size (Pango::SCALE * subject_font_size);
    font_description.set_family (font_family);
    font_metrics = pango_cr->get_metrics (font_description);

    int char_width = font_metrics.get_approximate_char_width () / Pango::SCALE;
    padding = char_width;
    left_icons_size = char_width;
    left_icons_width = char_width;

    thread->refresh ();

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    date_start          = left_icons_width_n * left_icons_width +
        (left_icons_width_n-1) * left_icons_padding + padding;
    date_width          = char_width * date_len;
    message_count_width = char_width * message_count_len;
    message_count_start = date_start + date_width + padding;
    authors_width       = char_width * authors_len;
    authors_start       = message_count_start + message_count_width + padding;
    tags_width          = char_width * tags_len;
    tags_start          = authors_start + authors_width + padding;
    subject_start       = tags_start + tags_width + padding;

    content_height = calculate_height (widget);
    height         = content_height + line_spacing;

    render_background (cr, widget, background_area, flags);
    render_date (cr, widget, cell_area); // returns height

    if (thread->total_messages > 1)
      render_message_count (cr, widget, cell_area);

    render_authors (cr, widget, cell_area);

    tags_width = render_tags (cr, widget, cell_area); // returns width
    subject_start = tags_start + tags_width / Pango::SCALE + ((tags_width > 0) ? padding : 0);

    render_subject (cr, widget, cell_area);

    /*
    if (!last)
      render_delimiter (cr, widget, cell_area);
    */

    if (thread->starred)
      render_starred (cr, widget, cell_area);

    if (thread->attachment)
      render_attachment (cr, widget, cell_area);

  }

  void ThreadIndexListCellRenderer::render_background ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &background_area,
      Gtk::CellRendererState flags) {

    int y = background_area.get_y ();
    int x = background_area.get_x ();

    int w = background_area.get_width ();
    int h = background_area.get_height ();

    Gdk::Color gray;
    if (flags & Gtk::CELL_RENDERER_SELECTED != 0) {
      gray.set_grey_p (.7);
    } else {
      gray.set_grey_p (1.);
    }
    cr->set_source_rgb (gray.get_red_p(), gray.get_green_p(), gray.get_blue_p());

    cr->rectangle (x, y, w, h);
    cr->fill ();

  } // }}}

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

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();

    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    ustring tag_string = VectorUtils::concat_tags (thread->tags);
    tag_string = tag_string.substr (0, tags_len);

    pango_layout->set_markup ("<span font_style=\"italic\"  color=\"#31587a\">" + tag_string + "</span>");

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(content_height / 2) - ((h / Pango::SCALE) / 2));

    cr->move_to (cell_area.get_x() + tags_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

    return w;

  } // }}}

  int ThreadIndexListCellRenderer::render_date ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    time_t newest = thread->newest_date;
    struct tm * timeinfo;
    char buf[80];
    timeinfo = localtime (&newest);
    strftime (buf, 80, "%D %R", timeinfo);

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (buf);

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

    return h; // return height!

  } // }}}

  void ThreadIndexListCellRenderer::render_message_count ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

# define BUFLEN 24
    char buf[BUFLEN];
    snprintf (buf, BUFLEN, "(%d)", thread->total_messages);


    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (buf);

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
      authors = VectorUtils::concat_authors (thread->authors);
    }

    if (authors.size() >= authors_len) {
      authors = authors.substr (0, authors_len);
      authors += ".";
    }

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (authors);

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

  int ThreadIndexListCellRenderer::calculate_height (Gtk::Widget &widget) const {
    /* figure out font height */
    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("TEST HEIGHT STRING");

    pango_layout->set_font_description (font_description);

    int w, h;
    pango_layout->get_size (w, h);
    int content_height = h / Pango::SCALE;
    return content_height;
  }

  /* cellrenderer overloads {{{ */
  void ThreadIndexListCellRenderer::get_preferred_height_vfunc (
      Gtk::Widget& widget,
      int& minimum_height,
      int& natural_height) const {

    int height = calculate_height (widget) + line_spacing;

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

