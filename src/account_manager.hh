# pragma once

# include <vector>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class Account {
    public:
      ustring name;
      ustring email;
      ustring gpgkey;
      ustring sendmail;
  };

  class AccountManager {
    public:
      static void initialize ();
      static void deinitialize ();

      static vector<Account> accounts;
      static int default_account;
  };
}

