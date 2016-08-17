# include "astroid.hh"

# include <boost/log/trivial.hpp>
# include <boost/log/sinks/basic_sink_backend.hpp>
# include <boost/log/sinks/sync_frontend.hpp>
# include <boost/log/sinks/frontend_requirements.hpp>
# include <boost/log/expressions.hpp>
# include <boost/log/support/date_time.hpp>

# include "log_view.hh"

namespace logging = boost::log;
namespace sinks   = boost::log::sinks;
namespace expr    = boost::log::expressions;


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

    /* register with boost::log */
    auto core = logging::core::get ();
    lv = std::shared_ptr<LogViewSink> (new LogViewSink (this));
    sink = boost::shared_ptr<lv_sink_t> (new lv_sink_t (lv));
    core->add_sink (sink);

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

  void LogView::pre_close () {
    auto core = logging::core::get ();
    core->remove_sink (sink);
    sink.reset ();
    lv.reset ();
  }

  void LogView::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void LogView::release_modal () {
    remove_modal_grab ();
  }

  LogViewSink::LogViewSink (LogView * lv) {
    log_view = lv;
  }

  LogViewSink::LogViewSink (const std::shared_ptr<LogViewSink> &lvs) {
    log_view = lvs->log_view;
  }

  void LogViewSink::consume (logging::record_view const& rec, string_type const& message) {
    /* probably need to do this on gui thread */
    auto iter = log_view->store->append();
    auto row  = *iter;

    auto lvl = rec[logging::trivial::severity];
    auto ts  = rec["TimeStamp"].extract<boost::posix_time::ptime> ();

    ustring l = ustring::compose (
        "<i>[%1]</i> %2: %3",
        lvl,
        boost::posix_time::to_simple_string (*ts),
        Glib::Markup::escape_text(message));

    if (lvl == logging::trivial::error) {
      row[log_view->m_columns.m_col_str] = "<span color=\"red\">" + l + "</span>";
    } else if (lvl == logging::trivial::warning) {
      row[log_view->m_columns.m_col_str] = "<span color=\"pink\">" + l + "</span>";
    } else if (lvl == logging::trivial::info) {
      row[log_view->m_columns.m_col_str] = l;
    } else {
      row[log_view->m_columns.m_col_str] = "<span color=\"gray\">" + l + "</span>";
    }

    auto path = log_view->store->get_path (iter);
    log_view->tv.scroll_to_row (path);
    log_view->tv.set_cursor (path);
  }

}

