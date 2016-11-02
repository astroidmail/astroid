# include "astroid.hh"

# include <deque>
# include <mutex>
# include <iostream>

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
    lv = boost::shared_ptr<LogViewSink> (new LogViewSink (this));
    sink = boost::shared_ptr<lv_sink_t> (new lv_sink_t (lv));
    core->add_sink (sink);

    msgs_d.connect (sigc::mem_fun (this, &LogView::consume));

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

  void LogView::consume () {
    std::unique_lock<std::mutex> lk (msgs_m);

    while (!msgs.empty ()) {

      auto p = msgs.front ();
      msgs.pop_front ();

      lk.unlock ();

      auto iter = store->append();
      auto row  = *iter;

      row[m_columns.m_col_str] = p;

      auto path = store->get_path (iter);
      tv.scroll_to_row (path);
      tv.set_cursor (path);

      lk.lock (); // for while
    }

    lk.unlock (); // for while
  }

  LogViewSink::LogViewSink (LogView * lv) {
    log_view = lv;
  }

  LogViewSink::LogViewSink (const std::shared_ptr<LogViewSink> &lvs) {
    log_view = lvs->log_view;
  }

  void LogViewSink::consume (logging::record_view const& rec, string_type const& message) {

    auto lvl = rec[logging::trivial::severity];

    auto ts  = logging::extract<boost::posix_time::ptime> ("TimeStamp",rec);
    boost::posix_time::time_facet * f = new boost::posix_time::time_facet ("%H:%M:%S.%f");

    std::ostringstream s;
    s.imbue (std::locale (s.getloc (), f));

    s << "<i>[" << std::setw (6) << lvl << "]</i> ";
    s << *ts << ": " << Glib::Markup::escape_text (message);

    ustring l = s.str ();

    if (lvl == logging::trivial::error) {
      l = "<span color=\"red\">" + l + "</span>";
    } else if (lvl == logging::trivial::warning) {
      l = "<span color=\"pink\">" + l + "</span>";
    } else if (lvl == logging::trivial::info) {
      l = l;
    } else {
      l = "<span color=\"gray\">" + l + "</span>";
    }

    std::unique_lock <std::mutex> lk (log_view->msgs_m);
    bool notify = log_view->msgs.empty ();

    log_view->msgs.push_back (l);

    lk.unlock ();

    if (notify) log_view->msgs_d.emit ();

  }

}

