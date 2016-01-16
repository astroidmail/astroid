# pragma once

# include <vector>

# include "astroid.hh"
# include "proto.hh"
# include "config.hh"

namespace Astroid {
  class Contacts {
    public:
      Contacts ();
      ~Contacts ();

      ptree contacts_config;

      bool enable_lbdb;
      string lbdb_cmd;

      int recent_contacts_load;
      string recent_query;
      void load_recent_contacts (int);

      void add_contact (ustring);

      std::vector<ustring> contacts;
      std::vector<ustring>::iterator search_external (ustring);

      class ContactCompletion : public Gtk::EntryCompletion {
        public:
          ContactCompletion ();

          Contacts * contacts;

          // tree model columns, for the EntryCompletion's filter model
          class ModelColumns : public Gtk::TreeModel::ColumnRecord
          {
            public:

              ModelColumns ()
              { add(m_id); add(m_cont); }

              Gtk::TreeModelColumn<unsigned int>  m_id;
              Gtk::TreeModelColumn<Glib::ustring> m_cont;
          };

          ModelColumns m_columns;
          refptr<Gtk::ListStore> completion_model;

          void execute_search ();
          ustring get_partial_contact (ustring);

          bool match (const ustring&, const
              Gtk::TreeModel::const_iterator&);

          bool on_match_selected(const Gtk::TreeModel::iterator& iter);
      };
  };
}

