# include "proto.hh"
# include "astroid.hh"

# include "log.hh"

# include "log_view.hh"

using namespace std;

namespace Astroid {
  LogView::LogView () {
    tab_widget = Gtk::manage(new Gtk::Label ("Log"));

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

    log.add_log_view (this);
    log << debug << "log window ready." << endl;
  }

  LogView::~LogView () {
    log.del_log_view (this);
  }

  void LogView::log_line (LogLevel lvl, ustring time_str, ustring s) {
    auto iter = store->append();
    auto row  = *iter;

    ustring l = ustring::compose (
        "<i>[%1]</i> %2: %3",
        Log::level_string (lvl),
        time_str,
        s);

    if (lvl == error) {
      row[m_columns.m_col_str] = "<span color=\"red\">" + l + "</span>";
    } else if (lvl == warn) {
      row[m_columns.m_col_str] = "<span color=\"yellow\">" + l + "</span>";
    } else if (lvl == info) {
      row[m_columns.m_col_str] = l;
    } else {
      /* debug */
      row[m_columns.m_col_str] = "<span color=\"gray\">" + l + "</span>";
    }

    auto path = store->get_path (iter);
    tv.scroll_to_row (path);
    tv.set_cursor (path);
  }

  void LogView::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void LogView::release_modal () {
    remove_modal_grab ();
  }

  bool LogView::on_key_press_event (GdkEventKey * event) {
    switch (event->keyval) {
      case GDK_KEY_j:
      case GDK_KEY_Down:
        {
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
        }
        break;

      case GDK_KEY_k:
      case GDK_KEY_Up:
        {
          Gtk::TreePath path;
          Gtk::TreeViewColumn *c;
          tv.get_cursor (path, c);
          path.prev ();
          if (path) {
            tv.set_cursor (path);
          }
          return true;
        }
        break;

      case GDK_KEY_J:
        {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());
          return true;
        }

      case GDK_KEY_K:
        {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_step_increment ());
          return true;
        }

      case GDK_KEY_Home:
      case GDK_KEY_1:
        {
          /* select first */
          tv.set_cursor (Gtk::TreePath("0"));
          return true;
        }

      case GDK_KEY_End:
      case GDK_KEY_0:
        {
          /* select last */
          auto it = store->children().end ();
          auto p  = store->get_path (--it);
          tv.set_cursor (p);

          return true;
        }


    }
    return false;
  }

}

