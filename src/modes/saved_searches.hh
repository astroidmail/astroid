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

      static void save_query (ustring query);
      static void add_query_to_history (ustring query);

      static std::vector<ustring> get_history ();

      /* called by astroid destructor to save search history */
      static void init ();
      static void destruct ();

    private:
      static ptree load_searches ();
      static void write_back_searches (ptree);

      static std::vector<ustring> history;

      static Glib::Dispatcher m_reload;

      void on_thread_changed (Db *, ustring);
      void load_startup_queries ();
      void load_saved_searches ();
      void add_query (ustring, ustring, bool saved = false, bool history = false);

      void reload ();
      void refresh_stats ();
    private:
      void refresh_stats_db (Db *);
      bool needs_refresh = false;
    public:
      bool show_all_history = false;

      int page_jump_rows;

      void on_my_row_activated (const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *);

    protected:
      class ModelColumns : public Gtk::TreeModel::ColumnRecord
      {
        public:

          ModelColumns()
          {
            add (m_col_description);
            add (m_col_saved);
            add (m_col_history);
            add (m_col_name);
            add (m_col_query);
            add (m_col_unread_messages);
            add (m_col_total_messages);
          }

          Gtk::TreeModelColumn<bool>          m_col_description;
          Gtk::TreeModelColumn<bool>          m_col_saved;
          Gtk::TreeModelColumn<bool>          m_col_history;
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

