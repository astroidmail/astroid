# pragma once

# include <deque>
# include <mutex>

# include "proto.hh"
# include "astroid.hh"
# include <boost/log/trivial.hpp>
# include <boost/log/sinks/basic_sink_backend.hpp>
# include <boost/log/sinks/sync_frontend.hpp>
# include <boost/log/sinks/frontend_requirements.hpp>
namespace logging = boost::log;
namespace sinks   = boost::log::sinks;

# include "mode.hh"

namespace Astroid {
  class LogViewSink : public sinks::basic_formatted_sink_backend <
                  char,
                  sinks::synchronized_feeding>
  {
    public:
      LogViewSink (LogView *);
      LogViewSink (const std::shared_ptr<LogViewSink> &lvs);

      void consume (logging::record_view const& rec, string_type const& message);

      LogView * log_view;
  };

  typedef sinks::synchronous_sink <LogViewSink> lv_sink_t;

  class LogView : public Mode
  {
    friend LogViewSink;

    public:
      LogView (MainWindow *);

      void grab_modal ()    override;
      void release_modal () override;
      void pre_close ()     override;

      boost::shared_ptr <LogViewSink> lv;
      boost::shared_ptr <lv_sink_t> sink;

    protected:
      class ModelColumns : public Gtk::TreeModel::ColumnRecord
      {
        public:

          ModelColumns()
          { add(m_col_str);}

          Gtk::TreeModelColumn<Glib::ustring> m_col_str;
      };

      ModelColumns m_columns;

      Gtk::TreeView tv;
      Gtk::ScrolledWindow scroll;
      refptr<Gtk::ListStore> store;

      /* consume log on GUI thread */
      std::deque<ustring> msgs;

      std::mutex msgs_m;
      Glib::Dispatcher msgs_d;

      void consume ();
  };

}

