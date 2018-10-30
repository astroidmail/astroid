/* command bar
 *
 * is sub classed from Gtk::SearchBar, provides a command and search bar
 * with an entry that takes completions.
 *
 * will in different modes accept full searches, buffer searches.
 *
 */

# pragma once

# include <gtkmm.h>

# include <functional>

# include "astroid.hh"
# include "proto.hh"

namespace Astroid {
  class CommandBar : public Gtk::SearchBar {
    public:
      enum CommandMode {
        Search = 0,
        SearchText,
        Filter,
        Tag,        /* apply or remove tags */
        DiffTag,    /* apply or remove tags using + or - */
        AttachMids,
      };

      CommandBar ();

      MainWindow * main_window;
      CommandMode mode;

      Gtk::Box hbox;
      Gtk::Label mode_label;
      Gtk::SearchEntry entry;

      enum EditMode {
        Tags,
        Chars
      };
      EditMode edit_mode;

      void set_main_window (MainWindow *);

      void on_entry_activated ();
      std::function<void(ustring)> callback;

      void enable_command (CommandMode, ustring cmd, std::function<void(ustring)>);
      void enable_command (CommandMode, ustring title, ustring cmd, std::function<void(ustring)>);

      ustring get_text ();
      void    set_text (ustring);

      void    start_generic (ustring);

      /* relay to search bar event handler */
      bool command_handle_event (GdkEventKey *);
      bool entry_key_press (GdkEventKey *);
      void entry_changed ();

    private:
      void reset_bar ();
      void setup_bar_labels ();

      class GenericCompletion : public Gtk::EntryCompletion {
        public:
          refptr<Gtk::ListStore> completion_model;

          virtual bool match (const ustring&, const
              Gtk::TreeModel::const_iterator&);

          virtual bool on_match_selected(const Gtk::TreeModel::iterator& iter);

          /* get the next match in the list */
          virtual void match_next ();
      };

      refptr<GenericCompletion> current_completion;

      /********************
       * Tag editing
       ********************/
      void start_tagging (ustring);

      class TagCompletion : public GenericCompletion {
        public:
          TagCompletion ();

          void load_tags (std::vector<ustring>);
          std::vector<ustring> tags; // must be sorted

          // tree model columns, for the EntryCompletion's filter model
          class ModelColumns : public Gtk::TreeModel::ColumnRecord
          {
            public:

              ModelColumns ()
              { add(m_tag); }

              Gtk::TreeModelColumn<Glib::ustring> m_tag;
          };

          ModelColumns m_columns;

          ustring break_on = ", ";
          ustring get_partial_tag (ustring_sz&);

          bool match (const ustring&, const
              Gtk::TreeModel::const_iterator&) override;

          bool on_match_selected(const Gtk::TreeModel::iterator& iter) override;

          /* color tags */
          static Gdk::RGBA  canvas_color;
          static bool       canvas_color_set;

          virtual void color_tags ();
          void color_tag (ustring tg, ustring_sz start, Pango::AttrList &attrs);
      };

      refptr<TagCompletion> tag_completion;

      /********************
       * Completer for diff Tag editing:
       *
       * +tag1 -tag2
       ********************
       */

      void start_difftagging (ustring);
      class DiffTagCompletion : public TagCompletion {
        public:
          DiffTagCompletion ();
      };
      refptr<DiffTagCompletion> difftag_completion;


      /********************
       * Search completion
       ********************/
      void start_searching (ustring);

      class SearchCompletion : public TagCompletion {
        public:
          SearchCompletion ();

          /* original text when browsing through search history */
          void load_history ();
          ustring orig_text = "";
          unsigned int history_pos;
          std::vector <ustring> history;

          bool get_partial_tag (ustring&, ustring_sz&);

          bool match (const ustring&, const
              Gtk::TreeModel::const_iterator&) override;

          bool on_match_selected(const Gtk::TreeModel::iterator& iter) override;

          void color_tags () override;
      };

      refptr<SearchCompletion> search_completion;

      /********************
       * SearchText completion
       ********************/
      void start_text_searching (ustring);

      class SearchTextCompletion : public GenericCompletion {
        public:
          SearchTextCompletion ();

          /* the search text completion maintains only the queries that have
           * been done during this program execution, the state is not saved */

          // tree model columns, for the EntryCompletion's filter model
          class ModelColumns : public Gtk::TreeModel::ColumnRecord
          {
            public:

              ModelColumns ()
              { add(m_query); }

              Gtk::TreeModelColumn<Glib::ustring> m_query;
          };

          ModelColumns m_columns;

          void add_query (ustring);
          bool on_match_selected(const Gtk::TreeModel::iterator& iter) override;
      };

      refptr<SearchTextCompletion> text_search_completion;
  };
}
