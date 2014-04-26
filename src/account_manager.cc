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

    if (default_account >= accounts.size()) {
      cout << "ac: no default account set, using first." << endl;
      default_account = 0;
      accounts[0].isdefault = true;
    }
  }

  AccountManager::~AccountManager () {
    cout << "ac: deinitializing." << endl;

  }
}

