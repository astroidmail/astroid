/* command bar
 *
 * is sub classed from Gtk::SearchBar, provides a command and search bar
 * with an entry that takes completions.
 *
 * will in different modes accept full searches, buffer searches, commands
 * or arguments for commands.
 *
 */

# pragma once

# include <gtkmm.h>

# include <functional>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class CommandBar : public Gtk::SearchBar {
    public:
      enum CommandMode {
        Search = 0,
        Generic,
        Tag,        /* apply or remove tags */
      };

      CommandBar ();

      MainWindow * main_window;
      CommandMode mode;

      Gtk::Box hbox;
      Gtk::Label mode_label;
      Gtk::SearchEntry entry;

      void set_main_window (MainWindow *);

      void on_entry_activated ();
      function<void(ustring)> callback;

      void enable_command (CommandMode, ustring, function<void(ustring)>);
      void disable_command ();

      void handle_command (ustring);

      ustring get_text ();
      void set_text (ustring);

      /* relay to search bar event handler */
      bool command_handle_event (GdkEventKey *);
      bool entry_key_press (GdkEventKey *);

    private:
      void reset_bar ();

      /********************
       * Tag editing 
       ********************/
      vector<ustring> existing_tags; // a sorted list of existing tags
      void start_tagging (ustring);

      class TagCompletion : public Gtk::EntryCompletion {
        public:
          TagCompletion ();
          void load_tags (vector<ustring>);

          vector<ustring> tags;

          // tree model columns, for the EntryCompletion's filter model
          class ModelColumns : public Gtk::TreeModel::ColumnRecord
          {
            public:

              ModelColumns ()
              { add(m_tag); }

              Gtk::TreeModelColumn<Glib::ustring> m_tag;
          };

          ModelColumns m_columns;
          refptr<Gtk::ListStore> completion_model;

          ustring get_partial_tag (ustring, ustring_sz&);

          bool match (const ustring&, const
              Gtk::TreeModel::const_iterator&);

          bool on_match_selected(const Gtk::TreeModel::iterator& iter);
      };

      refptr<TagCompletion> tag_completion;

      /********************
       * Search completion 
       ********************/
      void start_searching (ustring);

      class SearchCompletion : public Gtk::EntryCompletion {
        public:
          SearchCompletion ();
          void load_tags (vector<ustring>);

          vector<ustring> tags;

          // tree model columns, for the EntryCompletion's filter model
          class ModelColumns : public Gtk::TreeModel::ColumnRecord
          {
            public:

              ModelColumns ()
              { add(m_tag); }

              Gtk::TreeModelColumn<Glib::ustring> m_tag;
          };

          ModelColumns m_columns;
          refptr<Gtk::ListStore> completion_model;

          bool get_partial_tag (ustring, ustring&, ustring_sz&);

          bool match (const ustring&, const
              Gtk::TreeModel::const_iterator&);

          bool on_match_selected(const Gtk::TreeModel::iterator& iter);
      };

      refptr<SearchCompletion> search_completion;
  };
}
