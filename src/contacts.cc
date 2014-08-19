# include <iostream>
# include <vector>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "db.hh"
# include "config.hh"
# include "contacts.hh"
# include "utils/utils.hh"

using namespace std;

namespace Astroid {
  Contacts::Contacts () {
    cout << "ct: initializing.." << endl;

    contacts_config = astroid->config->config.get_child ("contacts");

    enable_lbdb = contacts_config.get<bool> ("lbdb.enable");
    lbdb_cmd    = contacts_config.get<string> ("lbdb.cmd");
    recent_contacts_load = contacts_config.get<int> ("recent.load");
    recent_query         = contacts_config.get<string> ("recent.query");



    load_recent_contacts (recent_contacts_load);
  }

  Contacts::~Contacts () {
    cout << "ct: deinitializing.." << endl;
  }

  void Contacts::load_recent_contacts (int n) {
    cout << "ct: loading " << n << " most recent contacts.." << endl;

    if (n <= 0) return;

    /* set up notmuch query */
    Db db (Db::DbMode::DATABASE_READ_ONLY);
    lock_guard<Db> lk (db);

    notmuch_query_t *     query;
    notmuch_messages_t *  messages;
    notmuch_message_t *   message;

    query =  notmuch_query_create (db.nm_db, recent_query.c_str());

    cout << "ct, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_messages (query) << " messages." << endl;

    messages = notmuch_query_search_messages (query);

    for (;
         notmuch_messages_valid (messages);
         notmuch_messages_move_to_next (messages)) {

      message = notmuch_messages_get (messages);

      ustring astr = ustring (notmuch_message_get_header (message, "From"));
      vector<ustring> aths = VectorUtils::split_and_trim (astr, ",");
      for (ustring &a : aths) add_contact (a);

      astr = ustring (notmuch_message_get_header (message, "To"));
      aths = VectorUtils::split_and_trim (astr, ",");
      for (ustring &a : aths) add_contact (a);

      if (static_cast<int>(contacts.size()) >= n) break;
    }

    notmuch_messages_destroy (messages);
    notmuch_query_destroy (query);
  }

  void Contacts::add_contact (ustring c) {
    /* TODO: use InternetAddress */
    UstringUtils::trim (c);
    if (c.empty()) return;

    if (find(contacts.begin (), contacts.end (), c) == contacts.end ()) {
      contacts.push_back (c);
      //cout << "ct: adding: " << c << endl;
    }
  }

  vector<ustring>::iterator Contacts::search_external (ustring) {
    return contacts.end ();
  }

  /********************
   * Contact Completion
   * ******************
   */

  Contacts::ContactCompletion::ContactCompletion ()
  {
    contacts = astroid->contacts;
    completion_model = Gtk::ListStore::create (m_columns);
    set_model (completion_model);
    set_text_column (m_columns.m_cont);
    set_match_func (sigc::mem_fun (*this,
          &Contacts::ContactCompletion::match));

    /* fill model with contacts */
    int i = 0;
    for (ustring &c : contacts->contacts) {
      auto row = *(completion_model->append ());
      row[m_columns.m_id] = i++;
      row[m_columns.m_cont] = c;
    }

    set_inline_completion (true);
  }

  /* search in external contacts for the current key
   * and populate contact list with results so that
   * they too can be used for matching. */
  void Contacts::ContactCompletion::execute_search () {
    /* TODO: use InternetAddress */
    ustring key = get_partial_contact (get_entry()->get_text ());

  }

  /* searches backwards to the previous ',' and extracts the
   * part of the partially entered contact to be matched on.
   */
  ustring Contacts::ContactCompletion::get_partial_contact (ustring c) {
    // TODO: handle completions when not editing the last contact
    uint n = c.find_last_of (",");
    if (n == ustring::npos) return c;

    c = c.substr (n+1, c.size());
    UstringUtils::trim_left (c);
    return c;
  }

  bool Contacts::ContactCompletion::match (
      const ustring& raw_key, const
      Gtk::TreeModel::const_iterator& iter)
  {
    if (iter)
    {
      ustring key = get_partial_contact (raw_key);

      Gtk::TreeModel::Row row = *iter;

      Glib::ustring::size_type key_length = key.size();
      Glib::ustring filter_string = row[m_columns.m_cont];

      Glib::ustring filter_string_start = filter_string.substr(0, key_length);

      // the key is lower-case, even if the user input is not.
      filter_string_start = filter_string_start.lowercase();

      if(key == filter_string_start)
        return true; // a match was found.
    }

    return false; // no match.
  }


  bool Contacts::ContactCompletion::on_match_selected (
      const Gtk::TreeModel::iterator& iter) {
    if (iter)
    {
      Gtk::Entry * entry = get_entry();

      Gtk::TreeModel::Row row = *iter;
      ustring completion = row[m_columns.m_cont];

      ustring t = entry->get_text ();
      uint n = t.find_last_of (",");
      if (n == ustring::npos) n = 0;

      t = t.substr (0, n);
      if (n == 0) t += completion;
      else        t += ", " + completion;

      entry->set_text (t);
      entry->set_position (-1);
    }

    return true;
  }
};

