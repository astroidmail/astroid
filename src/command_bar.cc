# include <iostream>
# include <functional>

# include <boost/tokenizer.hpp>

# include <gtkmm.h>
# include <gtkmm/widget.h>

# include "command_bar.hh"
# include "astroid.hh"
# include "main_window.hh"
# include "modes/mode.hh"
# include "modes/thread_index.hh"
# include "modes/help_mode.hh"
# include "utils/utils.hh"
# include "log.hh"
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

    /* set up tags */
    Db db (Db::DbMode::DATABASE_READ_ONLY);
    db.load_tags ();
    existing_tags = db.tags;

    tag_completion = refptr<TagCompletion>(new TagCompletion());
    search_completion = refptr<SearchCompletion>(new SearchCompletion());
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
    log << debug << "cb: cmd (in mode: " << mode << "): " << cmd << endl;
    set_search_mode (false); // emits changed -> disables search

    switch (mode) {
      case CommandMode::Search:
        if (cmd.size() > 0) {
          Mode * m = new ThreadIndex (main_window, cmd);

          main_window->add_mode (m);
        }
        break;

      case CommandMode::Generic:
        {
          handle_command (cmd);
        }
        break;

      case CommandMode::Tag:
        {
        }
        break;
    }

    if (callback != NULL) callback (cmd);
    callback = NULL;
  }

  void CommandBar::enable_command (CommandMode m, ustring cmd, function<void(ustring)> f) {
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

      case CommandMode::Generic:
        {
          mode_label.set_text ("");
          entry.set_icon_from_icon_name ("system-run-symbolic");
          entry.set_text (cmd);
        }
        break;

      case CommandMode::Tag:
        {
          mode_label.set_text ("Tags for thread:");
          entry.set_icon_from_icon_name ("system-run-symbolic");

          start_tagging (cmd);
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
    search_completion->load_tags (existing_tags);
    entry.set_completion (search_completion);
  }

  void CommandBar::start_tagging (ustring tagstring) {
    /* TODO: set up text tags */
    entry.set_text (tagstring);

    /* set up completion */
    tag_completion->load_tags (existing_tags);
    entry.set_completion (tag_completion);
  }

  void CommandBar::handle_command (ustring cmd) {
    log << debug << "cb: command: " << cmd << endl;

    UstringUtils::utokenizer tok (cmd);

# define adv_or_return it++; if (it == tok.end()) return;

    auto it = tok.begin ();
    if (it == tok.end ()) return;

    if (*it == "help") {
      main_window->add_mode (new HelpMode ());

    } else if (*it == "archive") {
      adv_or_return;

      if (*it == "thread") {
        adv_or_return;

        ustring thread_id = *it;

        log << info << "cb: toggle archive on thread: " << thread_id << endl;
      }

    } else {
      log << error  << "cb: unknown command: " << cmd << endl;
    }
  }

  void CommandBar::disable_command () {
  }

  bool CommandBar::command_handle_event (GdkEventKey * event) {
    return handle_event (event);
  }

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

    set_inline_completion (true);
    set_popup_completion (true);
    set_popup_single_match (false);
  }

  void CommandBar::TagCompletion::load_tags (vector<ustring> _tags) {
    tags = _tags;

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
  ustring CommandBar::TagCompletion::get_partial_tag (ustring c, uint &outpos) {
    Gtk::Entry * e = get_entry ();
    ustring in = e->get_text();
    c = in;
    int cursor     = e->get_position ();
    if (cursor == -1) cursor = c.size();

    outpos = c.find_last_of (",", cursor-1);
    if (outpos == ustring::npos) outpos = 0;


    uint endpos = c.find_first_of (", ", outpos+2);
    if (endpos == ustring::npos) endpos = c.size();

    c = c.substr (outpos+1, endpos-outpos-1);
    UstringUtils::trim_left (c);

    //log << debug << "cursor: " << cursor << ", outpos: " << outpos << ", in: " << in << ", o: " << c <<  endl;
    return c;
  }

  bool CommandBar::TagCompletion::match (
      const ustring& raw_key, const
      Gtk::TreeModel::const_iterator& iter)
  {
    uint    pos;
    ustring key = get_partial_tag (raw_key, pos);

    if (iter)
    {

      Gtk::TreeModel::Row row = *iter;

      Glib::ustring::size_type key_length = key.size();
      Glib::ustring filter_string = row[m_columns.m_tag];

      Glib::ustring filter_string_start = filter_string.substr(0, key_length);

      // the key is lower-case, even if the user input is not.
      filter_string_start = filter_string_start.lowercase();

      if(key == filter_string_start)
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
      uint    pos;
      ustring key = get_partial_tag (t, pos);

      //log << debug << "match selected: " << t << ", key: " << key << ", pos: " << pos << endl;

      if (pos == ustring::npos) pos = 0;
      else pos += 1; // now positioned after ','

      uint n = t.find_first_of (", ", pos); // break on these
      if (n == ustring::npos) n = t.size();

      ustring newt;
      newt = t.substr (0, pos);
      newt += " " + completion;
      newt += t.substr (pos+key.size()+1, t.size());

      entry->set_text (newt);
      entry->set_position (pos + completion.size()+1);
    }

    return true;
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

    set_inline_completion (true);
    set_popup_completion (true);
    set_popup_single_match (false);
  }

  void CommandBar::SearchCompletion::load_tags (vector<ustring> _tags) {
    tags = _tags;

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
  bool CommandBar::SearchCompletion::get_partial_tag (ustring c, ustring &out, uint &outpos) {
    // example input
    // (tag:asdf) and tag:asdfb and asdf
    //

    Gtk::Entry * e = get_entry ();
    uint cursor    = e->get_position ();

    outpos = c.rfind ("tag:");
    if (outpos == ustring::npos) return false;

    uint n = c.find_first_of (") ", outpos); // break completion on these chars
    if ((cursor >= n) || (cursor <= outpos)) return false;

    c = c.substr (outpos+4, c.size());
    UstringUtils::trim_left (c);

    out = c;

    return true;
  }

  bool CommandBar::SearchCompletion::match (
      const ustring& raw_key,
      const Gtk::TreeModel::const_iterator& iter)
  {
    ustring key;
    uint    pos;
    bool in_tag_search = get_partial_tag (raw_key, key, pos);

    if (in_tag_search && iter)
    {

      Gtk::TreeModel::Row row = *iter;

      Glib::ustring::size_type key_length = key.size();
      Glib::ustring filter_string = row[m_columns.m_tag];

      Glib::ustring filter_string_start = filter_string.substr(0, key_length);

      // the key is lower-case, even if the user input is not.
      filter_string_start = filter_string_start.lowercase();

      if(key == filter_string_start)
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
      uint    pos;
      bool in_tag_search = get_partial_tag (t, key, pos);

      if (in_tag_search) {

        pos += 4; // now positioned after tag:
        uint n = t.find_first_of (") "); // break on these

        if (n == ustring::npos) n = t.size();

        ustring newt = t.substr(0, pos);
        newt += completion;
        newt += t.substr (n, t.size());

        entry->set_text (newt);
        entry->set_position (pos + completion.size());
      }
    }

    return true;
  }
}

