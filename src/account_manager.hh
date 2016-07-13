# pragma once

# include <vector>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "proto.hh"
# include "utils/address.hh"

namespace bfs = boost::filesystem;

namespace Astroid {
  class Account {
    public:
      ustring id;
      ustring name;
      ustring email;
      ustring sendmail;

      bool isdefault;

      bool save_sent;
      ustring save_sent_to;
      std::vector<ustring> additional_sent_tags;
      ustring save_drafts_to;

      bfs::path signature_file;
      bool      signature_default_on;
      bool      signature_attach;
      bool      has_signature = false;

      ustring gpgkey;
      bool has_gpg = false;
      bool always_gpg_sign = false;

      ustring full_address ();

      bool operator== (Address &a);
  };

  class AccountManager {
    public:
      AccountManager ();
      ~AccountManager ();

      std::vector<Account> accounts;
      int default_account;
      Account * get_account_for_address (ustring);
      Account * get_account_for_address (Address);

      bool is_me (Address &);
  };
}

