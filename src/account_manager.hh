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
      bfs::path save_sent_to;
      std::vector<ustring> additional_sent_tags;
      bfs::path save_drafts_to;

      bool      signature_separate = false;
      bfs::path signature_file;
      bfs::path signature_file_markdown;
      bool      signature_default_on;
      bool      signature_attach;
      bool      has_signature = false;
      bool      has_signature_markdown = false;

      ustring   select_query = "";

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

      Account * get_assosciated_account (refptr<Message>);

      bool is_me (Address &);
  };
}

