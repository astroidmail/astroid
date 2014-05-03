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
    notmuch_query_t *   query;
    notmuch_messages_t * messages;
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
};

