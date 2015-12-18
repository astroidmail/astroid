# pragma once

# include <vector>

# include "astroid.hh"
# include "proto.hh"
# include "utils/address.hh"

using namespace std;

namespace Astroid {
  class Account {
    public:
      ustring id;
      ustring name;
      ustring email;
      ustring gpgkey;
      ustring sendmail;

      bool isdefault;

      bool save_sent;
      ustring save_sent_to;
      ustring save_drafts_to;

      ustring full_address ();

      bool operator== (Address &a);
  };

  class AccountManager {
    public:
      AccountManager ();
      ~AccountManager ();

      vector<Account> accounts;
      int default_account;
      Account * get_account_for_address (ustring);
      Account * get_account_for_address (Address);

      bool is_me (Address &);
  };
}

