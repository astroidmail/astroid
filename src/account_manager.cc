# include <iostream>
# include <vector>
# include <algorithm>

# include <boost/property_tree/ptree.hpp>

# include "astroid.hh"
# include "account_manager.hh"
# include "config.hh"
# include "log.hh"
# include "utils/ustring_utils.hh"

using namespace std;
using boost::property_tree::ptree;

namespace Astroid {
    AccountManager::AccountManager () {
    log << info << "ac: initializing accounts.." << endl;

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

      log << info << "ac: setup account: " << a->id << " for " << a->name << " (default: " << a->isdefault << ")" << endl;

      accounts.push_back (*a);
    }

    if (accounts.size () == 0) {
      log << error << "ac: no accounts defined!" << endl;
      throw runtime_error ("ac: no account defined!");
    }

    default_account = (find_if(accounts.begin(),
                               accounts.end (),
                               [&](Account &a) {
                                return a.isdefault;
                               }) - accounts.begin());

    if (default_account >= static_cast<int>(accounts.size())) {
      log << warn << "ac: no default account set, using first." << endl;
      default_account = 0;
      accounts[0].isdefault = true;
    }
  }

  Account * AccountManager::get_account_for_address (ustring address) {
    Address a = Address (address);
    return get_account_for_address (a);
  }

  Account * AccountManager::get_account_for_address (Address address) {
    for (auto &a : accounts) {
      if (a == address) {
        return &a;
      }
    }

    log << error << "ac: error: could not figure out which account: " << address.full_address() << " belongs to." << endl;
    return NULL;
  }

  AccountManager::~AccountManager () {
    log << info << "ac: deinitializing." << endl;
  }

  bool AccountManager::is_me (Address &a) {
    for (Account &ac : accounts) {
      if (ac == a) return true;
    }

    return false;
  }

  /* --------
   * Account
   * -------- */
  ustring Account::full_address () {
    return name + " <" + email + ">";
  }

  bool Account::operator== (Address &a) {
    ustring aa = Address (name, email).email ().lowercase ();
    ustring bb = a.email ().lowercase ();
    UstringUtils::trim (aa);
    UstringUtils::trim (bb);

    return (aa == bb);
  }
}

