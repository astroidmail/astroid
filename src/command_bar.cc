# include <iostream>
# include <functional>

# include <boost/tokenizer.hpp>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "command_bar.hh"
# include "astroid.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index/thread_index.hh"
# include "modes/help_mode.hh"
# include "modes/saved_searches.hh"
# include "utils/utils.hh"
# include "db.hh"

using namespace std;

namespace Astroid {
  CommandBar::CommandBar () {
    set_show_close_button ();
    connect_entry (entry);

    hbox.set_orientation (Gtk::ORIENTATION_HORIZONTAL);

    hbox.pack_start (mode_label, false, false, 5);
    hbox.pack_start (entry, true, true, 5);
    entry.set_size_request (600, -1);
    add(hbox);

    entry.signal_activate ().connect (
        sigc::mem_fun (*this, &CommandBar::on_entry_activated)
        );

    entry.signal_key_press_event ().connect (
        sigc::mem_fun (this, &CommandBar::entry_key_press)
        );

    entry.signal_changed ().connect (
        sigc::mem_fun (this, &CommandBar::entry_changed));

    /* set up tags */
    Db db (Db::DbMode::DATABASE_READ_ONLY);
    db.load_tags ();

    tag_completion = refptr<TagCompletion> (new TagCompletion());
    search_completion = refptr<SearchCompletion> (new SearchCompletion());
    text_search_completion = refptr<SearchTextCompletion> (new SearchTextCompletion ());
    difftag_completion = refptr<DiffTagCompletion> (new DiffTagCompletion ());
  }

  void CommandBar::set_main_window (MainWindow * mw) {
    main_window = mw;
  }

  ustring CommandBar::get_text () {
    return entry.get_text ();
  }

  void CommandBar::set_text (ustring txt) {
    entry.set_text (txt);
  }

  void CommandBar::on_entry_activated () {
    /* handle input */
    ustring cmd = get_text ();
    LOG (debug) << "cb: cmd (in mode: " << mode << "): " << cmd;

    switch (mode) {
      case CommandMode::Search:
        if (callback == NULL && (cmd.size() > 0)) {
          Mode * m = new ThreadIndex (main_window, cmd);

          /* add to saved searches */
          SavedSearches::add_query_to_history (cmd);

          main_window->add_mode (m);
        }
        break;

      case CommandMode::Filter:
      case CommandMode::SearchText:
        {
          text_search_completion->add_query (cmd);
        }
      case CommandMode::DiffTag:
      case CommandMode::Tag:
        {
        }
        break;
    }

    if (callback != NULL) callback (cmd);
    callback = NULL;

    set_search_mode (false); // emits changed -> disables search
  }

  void CommandBar::enable_command (
      CommandMode m,
      ustring title,
      ustring cmd,
      std::function<void(ustring)> f) {

    enable_command (m, cmd, f);

    mode_label.set_text (title);
  }

  void CommandBar::enable_command (
      CommandMode m,
      ustring cmd,
      std::function<void(ustring)> f) {
    mode = m;

    reset_bar ();

    switch (mode) {

      case CommandMode::Search:
        {
          mode_label.set_text ("");
          entry.set_icon_from_icon_name ("edit-find-symbolic");

          start_searching (cmd);
        }
        break;

      case CommandMode::Filter:
      case CommandMode::SearchText:
        {
          mode_label.set_text ("Find text");
          entry.set_icon_from_icon_name ("edit-find-symbolic");

          start_text_searching (cmd);
        }
        break;

      case CommandMode::Tag:
        {
          mode_label.set_text ("Change tags");
          entry.set_icon_from_icon_name ("system-run-symbolic");

          start_tagging (cmd);
        }
        break;

      case CommandMode::DiffTag:
        {
          mode_label.set_text ("Change tags (+/-)");
          entry.set_icon_from_icon_name ("system-run-symbolic");

          start_difftagging (cmd);
        }
        break;
    }

    callback = f;
    entry.set_position (-1);
    set_search_mode (true);
  }

  void CommandBar::reset_bar () {
    entry.set_completion (refptr<Gtk::EntryCompletion>());
  }

  void CommandBar::start_searching (ustring searchstring) {
    entry.set_text (searchstring);

    /* set up completion */
    search_completion->load_tags (Db::tags);
    search_completion->load_history ();
    search_completion->orig_text = "";
    search_completion->history_pos = 0;
    entry.set_completion (search_completion);
    current_completion = search_completion;
  }

  void CommandBar::start_text_searching (ustring searchstring) {
    entry.set_text (searchstring);

    /* set up completion */
    search_completion->load_history ();
    search_completion->orig_text = "";
    search_completion->history_pos = 0;
    entry.set_completion (text_search_completion);
    current_completion = text_search_completion;
  }

  void CommandBar::start_tagging (ustring tagstring) {
    /* TODO: set up text tags */
    entry.set_text (tagstring);

    /* set up completion */
    tag_completion->load_tags (Db::tags);
    entry.set_completion (tag_completion);
    current_completion = tag_completion;
  }

  void CommandBar::start_difftagging (ustring tagstring) {
    entry.set_text (tagstring);

    /* set up completion */
    difftag_completion->load_tags (Db::tags);
    entry.set_completion (difftag_completion);
    current_completion = difftag_completion;
  }

  bool CommandBar::entry_key_press (GdkEventKey * event) {
    switch (event->keyval) {
      case GDK_KEY_Tab:
        {
          /* grab the next completion */
          if (entry.get_completion()) {

            /* if completion is set, then current_completion should be set */
            current_completion->match_next ();
          }

          return true;
        }

      case GDK_KEY_Up:
        {
          if (mode == CommandMode::Search) {
            LOG (debug) << "cb: next history";

            if (search_completion->history.empty ()) return true;

            /* save original */
            if (search_completion->orig_text == "") {
              search_completion->orig_text = entry.get_text ();
            }

            if (search_completion->history.size() >= (search_completion->history_pos+1)) {

              search_completion->history_pos++;
              entry.set_text (search_completion->history[search_completion->history_pos -1]);
            }

            return true;
          } else {
            return false;
          }
        }

      case GDK_KEY_Down:
        {
          if (mode == CommandMode::Search) {
            LOG (debug) << "cb: previous history";

            if (search_completion->history.empty ()) return true;

            if (search_completion->history_pos > 0) {
              search_completion->history_pos--;
              if (search_completion->history_pos == 0) {
                entry.set_text (search_completion->orig_text);
                search_completion->orig_text = "";
              } else {
                entry.set_text (search_completion->history[search_completion->history_pos -1]);
              }
            }

            return true;
          } else {
            return false;
          }
        }

      default:
        {
          if (mode == CommandMode::Search) {
            /* reset history browsing */
            refptr<SearchCompletion> s = refptr<SearchCompletion>::cast_dynamic (current_completion);
            s->orig_text = "";
            s->history_pos = 0;

          }

          break;
        }
    }

    return false;
  }

  void CommandBar::entry_changed () {
    if (mode == CommandMode::Filter) {
      /* filter on the fly */

      ustring cmd = get_text ();
      if (callback != NULL) callback (cmd);
    }
  }

  bool CommandBar::command_handle_event (GdkEventKey * event) {
    return handle_event (event);
  }

  /********************
   * Generic Completion
   ********************/
  bool CommandBar::GenericCompletion::match (const ustring&, const Gtk::TreeModel::const_iterator&) {
    // do not call directly
    throw std::bad_function_call ();
  }

  bool CommandBar::GenericCompletion::on_match_selected(const Gtk::TreeModel::iterator&) {
    // do not call directly
    throw std::bad_function_call ();
  }

  /* get the next match in the list and use it to complete */
  void CommandBar::GenericCompletion::match_next () {
    /* LOG (debug) << "cb: completion: taking next match"; */

    Gtk::TreeIter fwditer = completion_model->get_iter ("0");

    Gtk::Entry * e = get_entry ();
    if (e == NULL) throw logic_error ("no entry associated with completion");

    ustring key = e->get_text ();

    while (fwditer) {
      if (match (key, fwditer)) {
        on_match_selected (fwditer);

        break;
      }

      fwditer++;
    }
  }

  /* take the next match in the list and return its index */
  /*
  unsigned int CommandBar::GenericCompletion::roll_completion (ustring_sz pos) {
    return 0;
  }
  */


  /********************
   * Tag Completion
   ********************/
  CommandBar::TagCompletion::TagCompletion ()
  {
    completion_model = Gtk::ListStore::create (m_columns);
    set_model (completion_model);
    set_text_column (m_columns.m_tag);
    set_match_func (sigc::mem_fun (*this,
          &CommandBar::TagCompletion::match));

    //set_inline_completion (true);
    //set_inline_selection (true);
    set_popup_completion (true);
    set_popup_single_match (true);
    set_minimum_key_length (1);
  }

  void CommandBar::TagCompletion::load_tags (vector<ustring> _tags) {
    tags = _tags;
    sort (tags.begin(), tags.end());

    completion_model->clear ();

    /* fill model with tags */
    for (ustring t : tags) {
      auto row = *(completion_model->append ());
      row[m_columns.m_tag] = t;
    }
  }

  /* searches backwards to the previous ',' and extracts the
   * part of the partially entered tag to be matched on.
   */
  ustring CommandBar::TagCompletion::get_partial_tag (ustring c, ustring_sz &outpos) {
    Gtk::Entry * e = get_entry ();
    if (e == NULL) return "";

    ustring in = e->get_text ();
    c = in;

    if (c.size() == 0) {
      outpos = 0;
      return c;
    }

    int cursor     = e->get_position ();
    if (cursor == -1) cursor = c.size();

    outpos = c.find_last_of (break_on, cursor-1);
    if (outpos == ustring::npos) outpos = 0;


    ustring_sz endpos = c.find_first_of (break_on, outpos+2);
    if (endpos == ustring::npos) endpos = c.size();

    if (outpos >= endpos) {
      return "";
    }

    if (break_on.find_first_of (c[outpos]) != ustring::npos) {
      outpos++; // skip delimiter
      endpos--;
    }

    c = c.substr (outpos, endpos);
    UstringUtils::trim_left (c);

    /* LOG (debug) << "cursor: " << cursor << ", outpos: " << outpos << ", in: " << in << ", o: " << c; */
    return c;
  }

  bool CommandBar::TagCompletion::match (
      const ustring& raw_key, const
      Gtk::TreeModel::const_iterator& iter)
  {
    if (iter)
    {

      ustring_sz pos;
      ustring key = get_partial_tag (raw_key, pos);

      Gtk::TreeModel::Row row = *iter;

      Glib::ustring::size_type key_length = key.size();
      Glib::ustring filter_string = row[m_columns.m_tag];

      Glib::ustring filter_string_start = filter_string.substr(0, key_length);

      // the key is lower-case, even if the user input is not.
      filter_string_start = filter_string_start.lowercase();

      if(key.lowercase () == filter_string_start)
        return true; // a match was found.
    }

    return false; // no match.
  }


  bool CommandBar::TagCompletion::on_match_selected (
      const Gtk::TreeModel::iterator& iter) {
    if (iter)
    {
      Gtk::Entry * entry = get_entry();

      Gtk::TreeModel::Row row = *iter;
      ustring completion = row[m_columns.m_tag];

      ustring t = entry->get_text ();
      ustring_sz pos;
      ustring key = get_partial_tag (t, pos);

      LOG (debug) << "match selected: " << t << ", key: " << key << ", pos: " << pos;

      /* pos is positioned at beginning of tag, after delimiter */
      if (pos == ustring::npos) pos = 0;
      /* else if (pos > 0) pos += 1; // now positioned after ',' */

      /* find end of key in input string */
      ustring_sz n = t.find_first_of (break_on, pos); // break on these
      if (n == ustring::npos) n = t.size();

      ustring newt;
      newt = t.substr (0, pos);
      // add space between ',' and tag if this is not the first or there is already a space there
      /* if (pos > 0 && *(newt.end()) != ' ') newt += " "; */

      newt += completion;

      // add remainder of text field, unless we are at the end of the line
      if (pos + key.size() < t.size()) {
        newt += t.substr (pos+key.size() + ((pos > 0) ? 1 : 0), t.size());
      }

      entry->set_text (newt);
      entry->set_position (pos + completion.size()+((pos > 0) ? 1 : 0));
    }

    return true;
  }


  /********************
   * Diff tagging
   ********************/
  CommandBar::DiffTagCompletion::DiffTagCompletion ()
  {
    completion_model = Gtk::ListStore::create (m_columns);
    set_model (completion_model);
    set_text_column (m_columns.m_tag);
    set_match_func (sigc::mem_fun (*this,
          &CommandBar::DiffTagCompletion::match));

    set_popup_completion (true);
    set_popup_single_match (true);
    set_minimum_key_length (1);
    break_on = "+- ";
  }

  void CommandBar::DiffTagCompletion::load_tags (vector<ustring> _tags) {
    tags = _tags;
    sort (tags.begin(), tags.end());

    completion_model->clear ();

    /* fill model with tags */
    for (ustring t : tags) {
      auto row = *(completion_model->append ());
      row[m_columns.m_tag] = t;
    }
  }

  /********************
   * Search Completion
   ********************/

  CommandBar::SearchCompletion::SearchCompletion ()
  {
    completion_model = Gtk::ListStore::create (m_columns);
    set_model (completion_model);
    set_text_column (m_columns.m_tag);
    set_match_func (sigc::mem_fun (*this,
          &CommandBar::SearchCompletion::match));

    //set_inline_completion (true);
    set_popup_completion (true);
    set_popup_single_match (true);
    set_minimum_key_length (1);
  }

  void CommandBar::SearchCompletion::load_history () {
    history = SavedSearches::get_history ();
    std::reverse (history.begin (), history.end ());
  }

  void CommandBar::SearchCompletion::load_tags (vector<ustring> _tags) {
    tags = _tags;
    sort (tags.begin(), tags.end());

    completion_model->clear ();

    /* fill model with tags */
    for (ustring t : tags) {
      auto row = *(completion_model->append ());
      row[m_columns.m_tag] = t;
    }
  }

  /* searches backwards to the previous ',' and extracts the
   * part of the partially entered tag to be matched on.
   */
  bool CommandBar::SearchCompletion::get_partial_tag (ustring c, ustring &out, ustring_sz &outpos) {
    // example input
    // (tag:asdf) and tag:asdfb and asdf
    //

    Gtk::Entry * e = get_entry ();
    if (e == NULL) return false;
    int cursor     = e->get_position ();

    ustring in = e->get_text ();
    c = in;

    if (cursor == -1) cursor = c.size();

    outpos = c.rfind ("tag:", cursor);
    if (outpos == ustring::npos) return false;

    /* check if we find any breaks between tag and current position */
    ustring_sz endpos = c.find_first_of (") ", outpos); // break completion on these chars
    if (endpos != ustring::npos) {
      if (endpos < static_cast<ustring_sz>(cursor)) {
        //LOG (debug) << "break between tag and cursor";
        return false;
      }
    } else {
      /* if we're at end, use remainder of string */
      endpos = c.size();
    }

    c = c.substr (outpos+4, endpos-outpos-4);
    UstringUtils::trim (c);

    out = c;

    // LOG (debug) << "cursor: " << cursor << ", outpos: " << outpos << ", in: " << in << ", o: " << c;

    return true;
  }

  bool CommandBar::SearchCompletion::match (
      const ustring& raw_key,
      const Gtk::TreeModel::const_iterator& iter)
  {
    ustring key;
    ustring_sz pos;
    bool in_tag_search = get_partial_tag (raw_key, key, pos);

    if (in_tag_search && iter)
    {

      Gtk::TreeModel::Row row = *iter;

      Glib::ustring::size_type key_length = key.size();
      Glib::ustring filter_string = row[m_columns.m_tag];

      Glib::ustring filter_string_start = filter_string.substr(0, key_length);

      // the key is lower-case, even if the user input is not.
      filter_string_start = filter_string_start.lowercase();

      if(key.lowercase () == filter_string_start)
        return true; // a match was found.
    }

    return false; // no match.
  }


  bool CommandBar::SearchCompletion::on_match_selected (
      const Gtk::TreeModel::iterator& iter) {
    if (iter)
    {
      Gtk::Entry * entry = get_entry();

      Gtk::TreeModel::Row row = *iter;
      ustring completion = row[m_columns.m_tag];

      ustring t = entry->get_text ();
      ustring key;
      ustring_sz    pos;
      bool in_tag_search = get_partial_tag (t, key, pos);

      //LOG (debug) << "match selected: " << t << ", in_tag: " << in_tag_search << ", key: " << key << ", pos: " << pos;

      if (in_tag_search) {

        pos += 4; // now positioned after tag:
        ustring_sz n = t.find_first_of (") "); // break on these

        if (n == ustring::npos) n = t.size();

        ustring newt = t.substr(0, pos);
        newt += completion;
        newt += t.substr (pos+key.size(), t.size());

        entry->set_text (newt);
        entry->set_position (pos + completion.size());
      }
    }

    return true;
  }

  /********************
   * SearchText Completion
   ********************/

  CommandBar::SearchTextCompletion::SearchTextCompletion ()
  {
    completion_model = Gtk::ListStore::create (m_columns);
    set_model (completion_model);
    set_text_column (m_columns.m_query);

    //set_inline_completion (true);
    set_popup_completion (true);
    set_popup_single_match (true);
    set_minimum_key_length (0);
  }

  void CommandBar::SearchTextCompletion::add_query (ustring c) {
    if (!c.empty ()) {
      auto row = *(completion_model->append ());
      row[m_columns.m_query] = c;
    }
  }

  bool CommandBar::SearchTextCompletion::on_match_selected (
      const Gtk::TreeModel::iterator& iter) {
    if (iter)
    {
      Gtk::Entry * entry = get_entry();

      Gtk::TreeModel::Row row = *iter;
      ustring completion = row[m_columns.m_query];

      entry->set_text (completion);
      entry->set_position (completion.size ());
    }

    return true;
  }
}

