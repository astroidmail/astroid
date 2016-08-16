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
# include <boost/property_tree/ptree.hpp>

# include <notmuch.h>

# include "thread_index.hh"
# include "thread_index_list_cell_renderer.hh"
# include "db.hh"
# include "utils/utils.hh"
# include "crypto.hh"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

using namespace std;
using boost::property_tree::ptree;

namespace Astroid {

  ThreadIndexListCellRenderer::ThreadIndexListCellRenderer (ThreadIndex * _ti) {
    ptree ti = astroid->config ("thread_index.cell");
    hidden_tags = VectorUtils::split_and_trim (ti.get<string> ("hidden_tags"), ",");
    std::sort (hidden_tags.begin (), hidden_tags.end ());

    thread_index = _ti;

    /* load font settings */
    font_desc_string = ti.get<string> ("font_description");
    if (font_desc_string == "" || font_desc_string == "default") {
      auto settings = Gio::Settings::create ("org.gnome.desktop.interface");
      font_desc_string = settings->get_string ("monospace-font-name");
    }

    font_description = Pango::FontDescription (font_desc_string);

    /* https://developer.gnome.org/pangomm/stable/classPango_1_1FontDescription.html#details */
    if (font_description.get_size () == 0) {
      LOG (warn) << "thread_index.cell.font_description: no size specified, expect weird behaviour.";
    }

    line_spacing  = ti.get<int> ("line_spacing");
    date_len      = ti.get<int> ("date_length");
    message_count_len = ti.get<int> ("message_count_length");
    authors_len   = ti.get<int> ("authors_length");
    tags_len      = ti.get<int> ("tags_length");

    subject_color = ti.get<string> ("subject_color");
    subject_color_selected = ti.get<string> ("subject_color_selected");
    background_color_selected = ti.get<string> ("background_color_selected");

  }

  void ThreadIndexListCellRenderer::render_vfunc (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget    &widget,
      const Gdk::Rectangle & background_area,
      const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags)
  {
    // calculate text width, we don't need to do this every time,
    // but we need access to the context.
    if (!height_set) {
      refptr<Pango::Context> pango_cr = widget.create_pango_context ();
      font_metrics = pango_cr->get_metrics (font_description);

      int char_width = font_metrics.get_approximate_char_width () / Pango::SCALE;
      padding = char_width;

      content_height  = calculate_height (widget);
      line_height     = content_height + line_spacing;
      height_set = true;

      left_icons_size  = content_height - (2 * left_icons_padding);
      left_icons_width = left_icons_size;

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

      height              = content_height + line_spacing;
    }

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    } else {
      font_description.set_weight (Pango::WEIGHT_NORMAL);
    }

    if (background_color_selected.length() > 0) {
        render_background (cr, widget, background_area, flags);
    }
    render_date (cr, widget, cell_area); // returns height

    if (thread->total_messages > 1)
      render_message_count (cr, widget, cell_area);

    render_authors (cr, widget, cell_area);

    tags_width = render_tags (cr, widget, cell_area, flags); // returns width
    subject_start = tags_start + tags_width / Pango::SCALE + ((tags_width > 0) ? padding : 0);

    render_subject (cr, widget, cell_area, flags);

    /*
    if (!last)
      render_delimiter (cr, widget, cell_area);
    */

    if (thread->flagged)
      render_flagged (cr, widget, cell_area);

    if (thread->attachment)
      render_attachment (cr, widget, cell_area);

    if (marked)
      render_marked (cr, widget, cell_area);

  }

  ThreadIndexListCellRenderer::~ThreadIndexListCellRenderer () {
    LOG (debug) << "til cr: deconstruct.";
  }

  void ThreadIndexListCellRenderer::render_background ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget & /* widget */,
      const Gdk::Rectangle &background_area,
      Gtk::CellRendererState flags) {

    int y = background_area.get_y ();
    int x = background_area.get_x ();

    int w = background_area.get_width ();
    int h = background_area.get_height ();

    if ((flags & Gtk::CELL_RENDERER_SELECTED) != 0) {
      Gdk::Color bg(background_color_selected);
      cr->set_source_rgb (bg.get_red_p(), bg.get_green_p(), bg.get_blue_p());
    } else {
      Gdk::Color gray;
      gray.set_grey_p (1.);
      cr->set_source_rgb (gray.get_red_p(), gray.get_green_p(), gray.get_blue_p());
    }

    cr->rectangle (x, y, w, h);
    cr->fill ();

  } // }}}

  /* render icons {{{ */
  void ThreadIndexListCellRenderer::render_marked (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget & /* widget */,
      const Gdk::Rectangle &cell_area ) {


    if (!marked_icon) {
      Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
          "object-select-symbolic",
          left_icons_size,
          Gtk::ICON_LOOKUP_USE_BUILTIN  | Gtk::ICON_LOOKUP_FORCE_SIZE);


      marked_icon = pixbuf->scale_simple (left_icons_size, left_icons_size,
          Gdk::INTERP_BILINEAR);
    }

    int y = cell_area.get_y() + left_icons_padding + line_spacing / 2;
    int x = cell_area.get_x();

    Gdk::Cairo::set_source_pixbuf (cr, marked_icon, x, y);

    cr->rectangle (x, y, left_icons_size, left_icons_size);
    cr->fill ();
  }

  void ThreadIndexListCellRenderer::render_flagged (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget & /* widget */,
      const Gdk::Rectangle &cell_area ) {


    if (!flagged_icon) {
      Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
          "starred-symbolic",
          left_icons_size,
          Gtk::ICON_LOOKUP_USE_BUILTIN  | Gtk::ICON_LOOKUP_FORCE_SIZE);

      flagged_icon = pixbuf->scale_simple (left_icons_size, left_icons_size,
          Gdk::INTERP_BILINEAR);
    }

    int y = cell_area.get_y() + left_icons_padding + line_spacing / 2;
    int x = cell_area.get_x() + left_icons_width + left_icons_padding;

    Gdk::Cairo::set_source_pixbuf (cr, flagged_icon, x, y);

    cr->rectangle (x, y, left_icons_size,left_icons_size);
    cr->fill ();
  }


  void ThreadIndexListCellRenderer::render_attachment (
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget & /* widget */,
      const Gdk::Rectangle &cell_area ) {


    if (!attachment_icon) {
      Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
          "mail-attachment-symbolic",
          left_icons_size,
          Gtk::ICON_LOOKUP_USE_BUILTIN | Gtk::ICON_LOOKUP_FORCE_SIZE);


      attachment_icon = pixbuf->scale_simple (left_icons_size, left_icons_size,
          Gdk::INTERP_BILINEAR);
    }

    int y = cell_area.get_y() + left_icons_padding + line_spacing / 2;
    int x = cell_area.get_x() + (2 * (left_icons_width + left_icons_padding));

    Gdk::Cairo::set_source_pixbuf (cr, attachment_icon, x, y);

    cr->rectangle (x, y, left_icons_size, left_icons_size);
    cr->fill ();
  } // }}}

  void ThreadIndexListCellRenderer::render_delimiter ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget & /*widget */,
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
      const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags) {

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("");

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();

    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());
    ustring color_str;
    if ((flags & Gtk::CELL_RENDERER_SELECTED) != 0) {
      color_str = subject_color_selected;
    } else {
      color_str = subject_color;
    }

    pango_layout->set_markup (ustring::compose ("<span color=\"%1\">%2</span>",
        color_str,
        Glib::Markup::escape_text(thread->subject)));

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(line_height / 2) - ((h / Pango::SCALE) / 2));

    cr->move_to (cell_area.get_x() + subject_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  } // }}}

  int ThreadIndexListCellRenderer::render_tags ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area,
      Gtk::CellRendererState flags) {

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("");

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();

    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    /* subtract hidden tags */
    vector<ustring> tags;
    set_difference (thread->tags.begin(),
                    thread->tags.end(),
                    hidden_tags.begin (),
                    hidden_tags.end (),
                    back_inserter(tags));

    ustring tag_string;

    Gdk::Color bg;

    if ((flags & Gtk::CELL_RENDERER_SELECTED) != 0) {
      bg = Gdk::Color (background_color_selected);
      cr->set_source_rgb (bg.get_red_p(), bg.get_green_p(), bg.get_blue_p());
    } else {
      bg.set_grey_p (1.);
    }

    /* first try plugin */
# ifndef DISABLE_PLUGINS
    if (!thread_index->plugins->format_tags (tags, bg.to_string (), (flags & Gtk::CELL_RENDERER_SELECTED) != 0, tag_string)) {
# endif

      unsigned char cv[3] = { (unsigned char) bg.get_red (),
                              (unsigned char) bg.get_green (),
                              (unsigned char) bg.get_blue () };

      tag_string = VectorUtils::concat_tags_color (tags, true, tags_len, cv);
# ifndef DISABLE_PLUGINS
    }
# endif

    pango_layout->set_markup (tag_string);

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(line_height / 2) - ((h / Pango::SCALE) / 2));

    cr->move_to (cell_area.get_x() + tags_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

    return w;

  } // }}}

  int ThreadIndexListCellRenderer::render_date ( // {{{
      const ::Cairo::RefPtr< ::Cairo::Context>&cr,
      Gtk::Widget &widget,
      const Gdk::Rectangle &cell_area ) {

    ustring date = Date::pretty_print (thread->newest_date);

    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout (date);

    pango_layout->set_font_description (font_description);

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();
    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(line_height / 2) - ((h / Pango::SCALE) / 2));

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
    int y = max(0,(line_height / 2) - ((h / Pango::SCALE) / 2));

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
      ustring an = get<0>(thread->authors[0]);

      if (static_cast<int>(an.size()) >= authors_len) {
        an = an.substr (0, authors_len);
        UstringUtils::trim_right(an);
        an += ".";
      }

      if (get<1>(thread->authors[0])) {
        authors = ustring::compose ("<b>%1</b>",
          Glib::Markup::escape_text (an));
      } else {
        authors = Glib::Markup::escape_text (an);
      }

    } else {
      /* show first names separated by comma */
      bool first = true;

      int len = 0;
      for (auto &a : thread->authors) {
        if (!first) len += 1; // comma

        ustring an = get<0>(a);

        an = an.substr (0, an.find_first_of (", @"));

        int tlen = static_cast<int>(an.size());
        if ((len + tlen) >= authors_len) {
          an = an.substr (0, authors_len - len);
          UstringUtils::trim_right (an);
          an += ".";
          tlen = authors_len - len;
        }

        len += tlen;

        if (!first) {
          authors += ",";
        } else {
          first = false;
        }

        if (get<1>(a)) {
          authors += ustring::compose ("<b>%1</b>", Glib::Markup::escape_text (an));
        } else {
          authors += Glib::Markup::escape_text (an);
        }


        if (len >= authors_len) {
          break;
        }
      }
    }


    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("");
    pango_layout->set_markup (authors);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_NORMAL);
    }

    pango_layout->set_font_description (font_description);

    if (thread->unread) {
      font_description.set_weight (Pango::WEIGHT_BOLD);
    }

    /* set color */
    Glib::RefPtr<Gtk::StyleContext> stylecontext = widget.get_style_context();
    Gdk::RGBA color = stylecontext->get_color(Gtk::STATE_FLAG_NORMAL);
    cr->set_source_rgb (color.get_red(), color.get_green(), color.get_blue());

    /* align in the middle */
    int w, h;
    pango_layout->get_size (w, h);
    int y = max(0,(line_height / 2) - ((h / Pango::SCALE) / 2));

    /* update subject start */
    //subject_start = date_start + (w / Pango::SCALE) + padding;

    cr->move_to (cell_area.get_x() + authors_start, cell_area.get_y() + y);
    pango_layout->show_in_cairo_context (cr);

  } // }}}

  /* cellrenderer overloads {{{ */
  int ThreadIndexListCellRenderer::get_height () {
    if (height_set) return line_height;
    else return 0;
  }

  int ThreadIndexListCellRenderer::calculate_height (Gtk::Widget &widget) const {
    if (height_set) return content_height;

    /* figure out font height */
    Glib::RefPtr<Pango::Layout> pango_layout = widget.create_pango_layout ("TEST HEIGHT STRING");

    pango_layout->set_font_description (font_description);

    int w, h;
    pango_layout->get_pixel_size (w, h);

    int content_height = h;

    return content_height;
  }

  Gtk::SizeRequestMode ThreadIndexListCellRenderer::get_request_mode_vfunc () const {
    return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
  }

  void ThreadIndexListCellRenderer::get_preferred_height_vfunc (
      Gtk::Widget& widget,
      int& minimum_height,
      int& natural_height) const {

    int height = calculate_height (widget) + line_spacing;

    minimum_height = height;
    natural_height = height;
  }

  void ThreadIndexListCellRenderer::get_preferred_height_for_width_vfunc (
      Gtk::Widget& widget,
      int /* width */,
      int& minimum_height,
      int& natural_height) const {

    /* wrap non-width dependent */
    get_preferred_height_vfunc (widget, minimum_height, natural_height);

  }

  void ThreadIndexListCellRenderer::get_preferred_width_vfunc (
      Gtk::Widget& /* widget*/,
      int& minimum_width,
      int& natural_width) const {

    minimum_width = 100;
    natural_width = minimum_width;
  }

  bool ThreadIndexListCellRenderer::activate_vfunc(
      GdkEvent* /* event */,
      Gtk::Widget& /* widget */,
      const Glib::ustring& /* path */,
      const Gdk::Rectangle& /* background_area */,
      const Gdk::Rectangle& /* cell_area */,
      Gtk::CellRendererState /* flags */)
  {
    return false;
  }
  // }}}

}

