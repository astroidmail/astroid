# pragma once

# include "mode.hh"

namespace Astroid {
  class SavedSearches : public Mode {
    public:
      SavedSearches (MainWindow *);

      void grab_modal () override;
      void release_modal () override;

      void load_startup_queries ();
      void add_query (ustring, ustring);

    protected:
      class ModelColumns : public Gtk::TreeModel::ColumnRecord
      {
        public:

          ModelColumns()
          { 
            add (m_col_name);
            add (m_col_query);
            add (m_col_unread_messages);
            add (m_col_total_messages);
          }

          Gtk::TreeModelColumn<Glib::ustring> m_col_name;
          Gtk::TreeModelColumn<Glib::ustring> m_col_query;
          Gtk::TreeModelColumn<int>           m_col_unread_messages;
          Gtk::TreeModelColumn<int>           m_col_total_messages;

      };

      ModelColumns m_columns;

      Gtk::TreeView tv;
      Gtk::ScrolledWindow scroll;
      refptr<Gtk::ListStore> store;
  };
}

