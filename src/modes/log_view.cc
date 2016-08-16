# include "astroid.hh"
# include <boost/log/trivial.hpp>

# include "log_view.hh"

using namespace std;
namespace logging = boost::log;

namespace Astroid {
  LogView::LogView (MainWindow * mw) : Mode (mw) {
    set_label ("Log");

    scroll.add (tv);
    pack_start (scroll);

    store = Gtk::ListStore::create (m_columns);
    tv.set_model (store);

    Gtk::CellRendererText * renderer_text = Gtk::manage (new Gtk::CellRendererText);
    renderer_text->property_family().set_value ("monospace");
    int cols_count = tv.append_column ("log entry", *renderer_text);
    Gtk::TreeViewColumn * pcolumn = tv.get_column (cols_count -1);
    if (pcolumn) {
      pcolumn->add_attribute (renderer_text->property_markup(), m_columns.m_col_str);
    }

    tv.set_headers_visible (false);
    tv.set_sensitive (true);
    set_sensitive (true);
    set_can_focus (true);

    show_all_children ();

    // TODO: add
    LOG (debug) << "log window ready.";

    keys.title = "Log view";
    keys.register_key ("j", { Key (GDK_KEY_Down) },
        "log.down",
        "Move cursor down",
        [&] (Key) {
          if (store->children().size() < 2)
            return true;

          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);

          path.next ();
          Gtk::TreeIter it = store->get_iter (path);

          if (it) {
            tv.set_cursor (path);
          }

          return true;
        });

    keys.register_key ("k", { Key (GDK_KEY_Up) },
        "log.up",
        "Move cursor up",
        [&] (Key) {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);
          path.prev ();
          if (path) {
            tv.set_cursor (path);
          }
          return true;
        });

    keys.register_key ("J",
        "log.page_down",
        "Page down",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());
          return true;
        });

    keys.register_key ("K",
        "log.page_up",
        "Page up",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_step_increment ());
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) },
        "log.home",
        "Scroll home",
        [&] (Key) {
          /* select first */
          tv.set_cursor (Gtk::TreePath("0"));
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) },
        "log.end",
        "Scroll to end",
        [&] (Key) {
          /* select last */
          auto it = store->children().end ();
          auto p  = store->get_path (--it);
          tv.set_cursor (p);

          return true;
        });

    keys.loghandle = false;
  }

  LogView::~LogView () {
    // TODO: remove
  }

  void LogView::log_line (logging::trivial::severity_level lvl, ustring time_str, ustring s) {
# if 0
    auto iter = store->append();
    auto row  = *iter;

    ustring l = ustring::compose (
        "<i>[%1]</i> %2: %3",
        Log::level_string (lvl),
        time_str,
        Glib::Markup::escape_text(s));

    if (lvl == logging::trivial::error) {
      row[m_columns.m_col_str] = "<span color=\"red\">" + l + "</span>";
    } else if (lvl == logging::trivial::warning) {
      row[m_columns.m_col_str] = "<span color=\"pink\">" + l + "</span>";
    } else if (lvl == logging::trivial::info) {
      row[m_columns.m_col_str] = l;
    } else {
      /* debug */
      row[m_columns.m_col_str] = "<span color=\"gray\">" + l + "</span>";
    }

    auto path = store->get_path (iter);
    tv.scroll_to_row (path);
    tv.set_cursor (path);
# endif
  }

  void LogView::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void LogView::release_modal () {
    remove_modal_grab ();
  }

}

