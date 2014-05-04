# include <iostream>
# include <vector>

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
    notmuch_query_t *     query;
    notmuch_messages_t *  messages;
    notmuch_message_t *   message;

    query =  notmuch_query_create (astroid->db->nm_db, recent_query.c_str());

    cout << "ct, query: " << notmuch_query_get_query_string (query) << ", approx: "
         << notmuch_query_count_messages (query) << " messages." << endl;

    messages = notmuch_query_search_messages (query);

    int i = 0;

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

      if (contacts.size() >= n) break;
    }

    notmuch_messages_destroy (messages);
    notmuch_query_destroy (query);
  }

  void Contacts::add_contact (ustring c) {
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
    //set_text_column (m_columns.m_cont);
    set_match_func (sigc::mem_fun (*this,
          &Contacts::ContactCompletion::match));

    /* fill model with contacts */
    int i = 0;
    for (ustring &c : contacts->contacts) {
      auto row = *(completion_model->append ());
      row[m_columns.m_id] = i++;
      row[m_columns.m_cont] = c;
    }
  }

  /* search in external contacts for the qurrent key
   * and populate contact list with results so that
   * they too can be used for matching. */
  void Contacts::ContactCompletion::execute_search () {
    ustring key = get_partial_contact (get_entry()->get_text ());

  }

  /* searches backwards to the previous , and extracts the
   * part of the partially entered contact to be matched on.
   */
  ustring Contacts::ContactCompletion::get_partial_contact (ustring c) {
    int n = c.find_last_of (",");
    if (n == ustring::npos) return c;

    return c.substr (n+1, c.size());
  }

  bool Contacts::ContactCompletion::match (
      const ustring& key, const
      Gtk::TreeModel::const_iterator& iter)
  {
    if (iter)
    {
      Gtk::TreeModel::Row row = *iter;

      Glib::ustring::size_type key_length = key.size();
      Glib::ustring filter_string = row[m_columns.m_cont];

      Glib::ustring filter_string_start = filter_string.substr(0, key_length);
      //The key is lower-case, even if the user input is not.
      filter_string_start = filter_string_start.lowercase();

      cout << "matching: " << key  << " against: " << filter_string_start << ": " << (key == filter_string_start) << endl;

      if(key == filter_string_start)
        return true; //A match was found.
    }

    return false; //No match.
  }
};

