# pragma once

# include "mode.hh"
# include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

namespace Astroid {
  class SavedSearches : public Mode {
    public:
      SavedSearches (MainWindow *);

      void grab_modal () override;
      void release_modal () override;

      void load_startup_queries ();
      void load_saved_searches ();
      void add_query (ustring, ustring);

      void reload ();
      void refresh_stats ();

      static void save_query (ustring query);

    private:
      ptree searches;

      static ptree load_searches ();
      static void write_back_searches (ptree);

      static Glib::Dispatcher m_reload;


    protected:
      class ModelColumns : public Gtk::TreeModel::ColumnRecord
      {
        public:

          ModelColumns()
          {
            add (m_col_description);
            add (m_col_name);
            add (m_col_query);
            add (m_col_unread_messages);
            add (m_col_total_messages);
          }

          Gtk::TreeModelColumn<bool>          m_col_description;
          Gtk::TreeModelColumn<Glib::ustring> m_col_name;
          Gtk::TreeModelColumn<Glib::ustring> m_col_query;
          Gtk::TreeModelColumn<Glib::ustring> m_col_unread_messages;
          Gtk::TreeModelColumn<Glib::ustring> m_col_total_messages;

      };

      ModelColumns m_columns;

      Gtk::TreeView tv;
      Gtk::ScrolledWindow scroll;
      refptr<Gtk::ListStore> store;
  };
}

