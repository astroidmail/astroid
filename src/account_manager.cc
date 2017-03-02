# include <iostream>
# include <vector>
# include <algorithm>

# include <boost/property_tree/ptree.hpp>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "account_manager.hh"
# include "config.hh"
# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"

using namespace std;
using boost::property_tree::ptree;
namespace bfs = boost::filesystem;

namespace Astroid {
    AccountManager::AccountManager () {
    LOG (info) << "ac: initializing accounts..";

    ptree apt = astroid->config ("accounts");

    for (const auto &kv : apt) {
      Account * a = new Account ();
      a->id = kv.first;

      a->name  = kv.second.get<string> ("name");
      a->email = kv.second.get<string> ("email");
      a->sendmail = kv.second.get<string> ("sendmail");

      a->isdefault = kv.second.get<bool> ("default");

      a->save_sent = kv.second.get<bool> ("save_sent");
      a->save_sent_to = kv.second.get<string> ("save_sent_to");
      ustring sent_tags_s = kv.second.get<string> ("additional_sent_tags");
      a->additional_sent_tags = VectorUtils::split_and_trim (sent_tags_s, ",");
      sort (a->additional_sent_tags.begin (), a->additional_sent_tags.end ());

      a->save_drafts_to = kv.second.get<string> ("save_drafts_to");

      a->signature_file = kv.second.get<string> ("signature_file");
      a->signature_default_on = kv.second.get<bool> ("signature_default_on");
      a->signature_attach     = kv.second.get<bool> ("signature_attach");

      if (a->signature_file.string ().size ()) {
        /* if relative, assume relative to config dir */
        if (!a->signature_file.is_absolute ()) {
          a->signature_file = astroid->standard_paths ().config_dir / a->signature_file;
        }


        if (bfs::exists (a->signature_file) &&
            bfs::is_regular_file (a->signature_file))
          a->has_signature = true;
      }

      a->gpgkey = kv.second.get<string> ("gpgkey");
      if (!a->gpgkey.empty()) {
        a->has_gpg = true;
        a->always_gpg_sign = kv.second.get<bool> ("always_gpg_sign");
      }


      LOG (info) << "ac: setup account: " << a->id << " for " << a->name << " (default: " << a->isdefault << ")";

      accounts.push_back (*a);
    }

    if (accounts.size () == 0) {
      LOG (error) << "ac: no accounts defined!";
      throw runtime_error ("ac: no account defined!");
    }

    default_account = (find_if(accounts.begin(),
                               accounts.end (),
                               [&](Account &a) {
                                return a.isdefault;
                               }) - accounts.begin());

    if (default_account >= static_cast<int>(accounts.size())) {
      LOG (warn) << "ac: no default account set, using first.";
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

    LOG (error) << "ac: error: could not figure out which account: " << address.full_address() << " belongs to.";
    return NULL;
  }

  AccountManager::~AccountManager () {
    /* LOG (info) << "ac: deinitializing."; */
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

