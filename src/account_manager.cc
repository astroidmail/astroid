# include <iostream>
# include <vector>
# include <algorithm>

# include <boost/property_tree/ptree.hpp>

# include "astroid.hh"
# include "account_manager.hh"
# include "config.hh"

using namespace std;
using boost::property_tree::ptree;

namespace Astroid {
    AccountManager::AccountManager () {
    cout << "ac: initializing accounts.." << endl;

    ptree apt = astroid->config->config.get_child ("accounts");

    for (const auto &kv : apt) {
      Account * a = new Account ();
      a->id = kv.first;

      a->name  = kv.second.get<string> ("name");
      a->email = kv.second.get<string> ("email");
      a->gpgkey = kv.second.get<string> ("gpgkey");
      a->sendmail = kv.second.get<string> ("sendmail");

      a->isdefault = kv.second.get<bool> ("default");

      a->save_sent = kv.second.get<bool> ("save_sent");
      a->save_sent_to = kv.second.get<string> ("save_sent_to");

      cout << "ac: setup account: " << a->id << " for " << a->name << " (default: " << a->isdefault << ")" << endl;

      accounts.push_back (*a);
    }

    if (accounts.size () == 0) {
      cerr << "ac: no accounts defined!" << endl;
      exit (1);
    }

    default_account = (find_if(accounts.begin(),
                               accounts.end (),
                               [&](Account &a) {
                                return a.isdefault;
                               }) - accounts.begin());

    if (default_account >= static_cast<int>(accounts.size())) {
      cout << "ac: no default account set, using first." << endl;
      default_account = 0;
      accounts[0].isdefault = true;
    }
  }

  Account * AccountManager::get_account_for_address (ustring address) {
    for (auto &a : accounts) {
      if (a.full_address() == address) {
        return &a;
      }
    }

    cout << "ac: error: could not figure out which account: " << address << " belongs to." << endl;
    return NULL;
  }

  AccountManager::~AccountManager () {
    cout << "ac: deinitializing." << endl;

  }

  /* --------
   * Account
   * -------- */
  ustring Account::full_address () {
    return name + " <" + email + ">";
  }
}

