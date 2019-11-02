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
    add(hbox);

    entry.signal_activate ().connect (
        sigc::mem_fun (*this, &CommandBar::on_entry_activated)
        );

    entry.signal_event ().connect ([&] (GdkEvent* event)->bool {
        if (event->type == GDK_KEY_PRESS) {
            return entry_key_press ((GdkEventKey *) event);
        } else {
            return false;
        }
    });

    entry.signal_changed ().connect (
        sigc::mem_fun (this, &CommandBar::entry_changed));

    /* set up tags */
    {
      Db db (Db::DbMode::DATABASE_READ_ONLY);
      db.load_tags ();
    }

    tag_completion          = refptr<TagCompletion> (new TagCompletion());
    search_completion       = refptr<SearchCompletion> (new SearchCompletion());
    text_search_completion  = refptr<SearchTextCompletion> (new SearchTextCompletion ());
    difftag_completion      = refptr<DiffTagCompletion> (new DiffTagCompletion ());
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
      case CommandMode::AttachMids:
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

    ustring edit_mode_str = astroid->config ().get<string> ("general.tagbar_move");
    if (edit_mode_str == "tag") {
      edit_mode = EditMode::Tags;
    } else {
      edit_mode = EditMode::Chars;
    }

    reset_bar ();

    switch (mode) {

      case CommandMode::AttachMids:
        {
          entry.set_icon_from_icon_name ("mail-attachment-symbolic");
          start_generic (cmd);
        }
        break;

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

  void CommandBar::start_generic (ustring cmd) {
    entry.set_completion (refptr<Gtk::EntryCompletion> ());
    current_completion.reset ();

    entry.set_text (cmd);
  }

  void CommandBar::start_searching (ustring searchstring) {
    /* set up completion */
    search_completion->load_tags (Db::tags);
    search_completion->load_history ();
    search_completion->orig_text = "";
    search_completion->history_pos = 0;
    entry.set_completion (search_completion);
    current_completion = search_completion;

    search_completion->color_tags (edit_mode);

    entry.set_text (searchstring);
  }

  void CommandBar::start_text_searching (ustring searchstring) {
    /* set up completion */
    search_completion->load_history ();
    search_completion->orig_text = "";
    search_completion->history_pos = 0;
    entry.set_completion (text_search_completion);
    current_completion = text_search_completion;

    entry.set_text (searchstring);
  }

  void CommandBar::start_tagging (ustring tagstring) {
    /* set up completion */
    tag_completion->load_tags (Db::tags);
    entry.set_completion (tag_completion);
    current_completion = tag_completion;

    entry.set_text (tagstring);

    tag_completion->color_tags (edit_mode);
  }

  void CommandBar::start_difftagging (ustring tagstring) {
    /* set up completion */
    difftag_completion->load_tags (Db::tags);
    entry.set_completion (difftag_completion);
    current_completion = difftag_completion;

    entry.set_text (tagstring);

    difftag_completion->color_tags (edit_mode);
  }

  bool CommandBar::entry_key_press (GdkEventKey * event) {
    LOG (debug) << "cb: got key: " << event->keyval;

    switch (event->keyval) {
      case GDK_KEY_Control_L:
      case GDK_KEY_Control_R:
        {
          edit_mode = edit_mode == EditMode::Tags ? EditMode::Chars : EditMode::Tags;
          entry_changed ();

          return true;
        }

      case GDK_KEY_Tab:
        {
          /* grab the next completion */
          if (entry.get_completion()) {

            /* if completion is set, then current_completion should be set */
            current_completion->match_next ();
          }

          return true;
        }

      case GDK_KEY_Left:
        {
          if (edit_mode == EditMode::Tags && (mode == CommandMode::Tag || mode == CommandMode::DiffTag)) {
            ustring txt = entry.get_text ();
            ustring_sz pos = entry.get_position ();
            // normal behavior if we're not in between tags
            if (txt[pos] != ' ' && pos > 0 && pos < txt.size ()) return false;
            // skip two spaces at the end
            if (pos == txt.size ()) pos--;
            if (pos > 0) pos--;
            while (txt[pos] != ' ' && pos > 0) pos--;
            entry.set_position (pos);
            return true;
          }
        }
        break;

      case GDK_KEY_BackSpace:
        {
          if (edit_mode == EditMode::Tags && (mode == CommandMode::Tag || mode == CommandMode::DiffTag)) {
            ustring txt = entry.get_text ();
            ustring_sz end = entry.get_position ();
            // normal behavior if we're not in between tags
            if (txt[end] != ' ' && end > 0 && end < txt.size ()) return false;
            // normal behavior if we're at the end and the character is not a
            // space -- assume editing tag
            if (txt[end] != ' ' && end == txt.size ()) return false;
            ustring_sz start = end;
            // skip two spaces at the end
            if (start == txt.size ()) start--;
            if (start > 0) start--;
            while (txt[start] != ' ' && start > 0) start--;
            entry.set_text (txt.substr (0, start) + txt.substr (end, txt.size () - end));
            entry.set_position (start);
            return true;
          }
        }
        break;

      case GDK_KEY_Right:
        {
          if (edit_mode == EditMode::Tags && (mode == CommandMode::Tag || mode == CommandMode::DiffTag)) {
            ustring txt = entry.get_text ();
            ustring_sz pos = entry.get_position ();
            // normal behavior if we're not in between tags
            if (txt[pos] != ' ' && pos > 0 && pos < txt.size ()) return false;
            if (pos < txt.size ()) pos++;
            while (txt[pos] != ' ' && pos < txt.size ()) pos++;
            entry.set_position (pos);
            return true;
          }
        }
        break;

      case GDK_KEY_Delete:
        {
          if (edit_mode == EditMode::Tags && (mode == CommandMode::Tag || mode == CommandMode::DiffTag)) {
            ustring txt = entry.get_text ();
            ustring_sz start = entry.get_position ();
            // normal behavior if we're not in between tags
            if (txt[start] != ' ' && start > 0 && start < txt.size ()) return false;
            ustring_sz end = start;
            if (end < txt.size ()) end++;
            while (txt[end] != ' ' && end < txt.size ()) end++;
            entry.set_text (txt.substr (0, start) + txt.substr (end, txt.size () - end));
            entry.set_position (start);
            return true;
          }
        }
        break;

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
          }
        }
        break;

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
          }
          if (mode == CommandMode::Tag || mode == CommandMode::DiffTag) {
            return true;
          }
        }
        break;

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

    } else if (mode == CommandMode::Tag || mode == CommandMode::DiffTag || mode == CommandMode::Search) {

      if (current_completion)
        refptr<TagCompletion>::cast_dynamic (current_completion)->color_tags (edit_mode);

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


  /********************
   * Tag Completion
   ********************/
  bool      CommandBar::TagCompletion::canvas_color_set = false;
  Gdk::RGBA CommandBar::TagCompletion::canvas_color;

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

  /* searches backwards to the previous ' ' and extracts the
   * part of the partially entered tag to be matched on.
   */
  ustring CommandBar::TagCompletion::get_partial_tag (ustring_sz &outpos) {
    Gtk::Entry * e = get_entry ();
    if (e == NULL) return "";

    ustring in = e->get_text ();

    if (in.size() == 0) {
      outpos = 0;
      return in;
    }

    int cursor     = e->get_position ();
    if (cursor == -1) cursor = in.size();

    outpos = in.find_last_of (break_on, cursor-1);
    if (outpos == ustring::npos) outpos = 0;


    ustring_sz endpos = in.find_first_of (break_on, outpos+2);
    if (endpos == ustring::npos) endpos = in.size();

    if (outpos >= endpos) {
      return "";
    }

    if (break_on.find_first_of (in[outpos]) != ustring::npos) {
      outpos++; // skip delimiter
      endpos--;
    }

    in = in.substr (outpos, endpos);
    UstringUtils::trim_left (in);

    /* LOG (debug) << "cursor: " << cursor << ", outpos: " << outpos << ", in: " << in << ", o: " << c; */
    return in;
  }

  bool CommandBar::TagCompletion::match (
      const ustring&, const
      Gtk::TreeModel::const_iterator& iter)
  {
    if (iter)
    {

      ustring_sz pos;
      ustring key = get_partial_tag (pos);

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
      ustring key = get_partial_tag (pos);

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

  void CommandBar::TagCompletion::color_tags (EditMode edit_mode) {
    Gtk::Entry * entry = get_entry ();
    if (entry == NULL) return;

    if (!canvas_color_set) {
      canvas_color     = entry->get_style_context()->get_background_color ();
      canvas_color_set = true;
    }

    ustring txt = entry->get_text ();

    /* set up attrlist */
    Pango::AttrList attrs;

    /* walk through unfinished tag list and style each tag */
    ustring_sz pos = 0;
    ustring_sz end = pos;
    ustring_sz len = txt.size ();

    while (pos < len) {
      /* find beginning of tag */
      pos = txt.find_first_not_of (break_on, pos);

      if (pos == ustring::npos) break;

      /* find end of tag */
      end = txt.find_first_of (break_on, pos);

      if (end == ustring::npos)
        end = txt.length ();

      ustring tag = txt.substr (pos, (end - pos));

      /* grapheme positions */
      ustring_sz gstart = txt.substr (0, pos).bytes ();

      color_tag (tag, gstart, attrs, edit_mode);

      pos = end+1;
    }

    entry->set_attributes (attrs);
  }

  void CommandBar::TagCompletion::color_tag (ustring tag,
    ustring_sz gstart, Pango::AttrList &attrs, EditMode edit_mode) {

    unsigned char cv[3] = { (unsigned char) (canvas_color.get_red_u ()   * 255 / 65535),
                            (unsigned char) (canvas_color.get_green_u () * 255 / 65535),
                            (unsigned char) (canvas_color.get_blue_u ()  * 255 / 65535) };

    auto colors = Utils::get_tag_color_rgba (tag, cv);

    auto fg = colors.first;
    auto bg = colors.second;

    ustring_sz gend = gstart + tag.bytes ();

    auto fga = Pango::Attribute::Attribute::create_attr_foreground (fg.get_red_u (), fg.get_green_u (), fg.get_blue_u ());
    fga.set_start_index (gstart);
    fga.set_end_index   (gend);

    auto bga = Pango::Attribute::Attribute::create_attr_background (bg.get_red_u (), bg.get_green_u (), bg.get_blue_u ());
    bga.set_start_index (gstart);
    bga.set_end_index   (gend);

    auto bgalpha = Pango::Attribute::Attribute::create_attr_background_alpha (bg.get_alpha_u ());
    bgalpha.set_start_index (gstart);
    bgalpha.set_end_index   (gend);

    if (edit_mode == EditMode::Chars) {
      auto underline = Pango::Attribute::create_attr_underline (Pango::UNDERLINE_SINGLE);
      underline.set_start_index (gstart);
      underline.set_end_index (gend);
      attrs.insert (underline);
    }

    attrs.insert (bga);
    attrs.insert (bgalpha);
    attrs.insert (fga);
  }


  /********************
   * Diff tagging
   ********************/
  CommandBar::DiffTagCompletion::DiffTagCompletion ()
  {
    break_on = "+- ";
  }


  /********************
   * Search Completion
   ********************/

  CommandBar::SearchCompletion::SearchCompletion ()
  {
    set_match_func (sigc::mem_fun (*this,
          &CommandBar::SearchCompletion::match));
  }

  void CommandBar::SearchCompletion::load_history () {
    history = SavedSearches::get_history ();
    std::reverse (history.begin (), history.end ());
  }

  /* searches backwards to the previous ',' and extracts the
   * part of the partially entered tag to be matched on.
   */
  bool CommandBar::SearchCompletion::get_partial_tag (ustring &out, ustring_sz &outpos) {
    // example input
    // (tag:asdf) and tag:asdfb and asdf
    //

    Gtk::Entry * e = get_entry ();
    if (e == NULL) return false;
    int cursor     = e->get_position ();

    ustring in = e->get_text ();

    if (cursor == -1) cursor = in.size();

    outpos = in.rfind ("tag:", cursor);
    if (outpos == ustring::npos) return false;

    /* check if we find any breaks between tag and current position */
    ustring_sz endpos = in.find_first_of (") ", outpos); // break completion on these chars
    if (endpos != ustring::npos) {
      if (endpos < static_cast<ustring_sz>(cursor)) {
        //LOG (debug) << "break between tag and cursor";
        return false;
      }
    } else {
      /* if we're at end, use remainder of string */
      endpos = in.size();
    }

    in = in.substr (outpos+4, endpos-outpos-4);
    UstringUtils::trim (in);

    out = in;

    // LOG (debug) << "cursor: " << cursor << ", outpos: " << outpos << ", in: " << in << ", o: " << c;

    return true;
  }

  bool CommandBar::SearchCompletion::match (
      const ustring&,
      const Gtk::TreeModel::const_iterator& iter)
  {
    ustring key;
    ustring_sz pos;
    bool in_tag_search = get_partial_tag (key, pos);

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
      bool in_tag_search = get_partial_tag (key, pos);

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

  void CommandBar::SearchCompletion::color_tags (EditMode edit_mode) {
    Gtk::Entry * entry = get_entry ();
    if (entry == NULL) return;

    if (!canvas_color_set) {
      canvas_color     = entry->get_style_context()->get_background_color ();
      canvas_color_set = true;
    }

    ustring txt = entry->get_text ();

    /* set up attrlist */
    Pango::AttrList attrs;

    /* walk through unfinished tag list and style each tag */
    ustring_sz pos = 0;
    ustring_sz end = pos;
    ustring_sz len = txt.size ();

    ustring break_on = " )";

    while (pos < len) {
      /* find beginning of tag */
      pos = txt.find ("tag:", pos);

      if (pos == ustring::npos) break;

      pos += 4;

      /* find end of tag */
      end = txt.find_first_of (break_on, pos);

      if (end == ustring::npos)
        end = txt.length ();

      ustring tag = txt.substr (pos, (end - pos));

      /* grapheme positions */
      ustring_sz gstart = txt.substr (0, pos).bytes ();

      color_tag (tag, gstart, attrs, edit_mode);

      pos = end+1;
    }

    entry->set_attributes (attrs);
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

