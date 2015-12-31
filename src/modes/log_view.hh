# pragma once

# include "proto.hh"
# include "astroid.hh"

# include "mode.hh"
# include "log.hh"

using namespace std;

namespace Astroid {
  class LogView : public Mode {
    public:
      LogView (MainWindow *);
      ~LogView ();

      void log_line (LogLevel, ustring, ustring);

      void grab_modal () override;
      void release_modal () override;

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
  };
}

