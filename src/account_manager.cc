# include <iostream>
# include <vector>
# include <algorithm>

# include <boost/property_tree/ptree.hpp>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "account_manager.hh"
# include "config.hh"
# include "db.hh"
# include "message_thread.hh"
# include "utils/ustring_utils.hh"
# include "utils/vector_utils.hh"
# include "utils/utils.hh"

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

      boost::optional<bool> bv;
      boost::optional<string> sv;

      a->name  =
        (sv = kv.second.get_optional<string> ("name"),
        (sv ? sv.get () : ""));

      a->email =
        (sv = kv.second.get_optional<string> ("email"),
        (sv ? sv.get () : ""));

      a->sendmail =
        (sv = kv.second.get_optional<string> ("sendmail"),
        (sv ? sv.get () : ""));

      a->isdefault =
        (bv = kv.second.get_optional<bool> ("default"),
        (bv ? bv.get () : false));

      a->save_sent =
        (bv = kv.second.get_optional<bool> ("save_sent"),
        (bv ? bv.get () : false));

      a->save_sent_to = Utils::expand(bfs::path (
            (sv = kv.second.get_optional<string> ("save_sent_to"),
            (sv ? sv.get () : ""))
             ));

      ustring sent_tags_s =
        (sv = kv.second.get_optional<string> ("additional_sent_tags"),
        (sv ? sv.get () : ""));

      a->additional_sent_tags = VectorUtils::split_and_trim (sent_tags_s, ",");
      sort (a->additional_sent_tags.begin (), a->additional_sent_tags.end ());

      a->save_drafts_to = Utils::expand(bfs::path (
            (sv = kv.second.get_optional<string> ("save_drafts_to"),
            (sv ? sv.get () : ""))
             ));

      a->signature_separate =
        (bv = kv.second.get_optional<bool> ("signature_separate"),
        (bv ? bv.get () : false));

      a->signature_file = Utils::expand(bfs::path (
            (sv = kv.second.get_optional<string> ("signature_file"),
            (sv ? sv.get () : ""))
             ));

      a->signature_file_markdown = Utils::expand(bfs::path (
            (sv = kv.second.get_optional<string> ("signature_file_markdown"),
            (sv ? sv.get () : ""))
             ));

      a->signature_default_on =
        (bv = kv.second.get_optional<bool> ("signature_default_on"),
        (bv ? bv.get () : false));

      a->signature_attach =
        (bv = kv.second.get_optional<bool> ("signature_attach"),
        (bv ? bv.get () : false));

      if (a->signature_file.string ().size ()) {
        /* if relative, assume relative to config dir */
        if (!a->signature_file.is_absolute ()) {
          a->signature_file = astroid->standard_paths ().config_dir / a->signature_file;
        }

        if (bfs::exists (a->signature_file) &&
            bfs::is_regular_file (a->signature_file))
          a->has_signature = true;
      }

      if (a->signature_file_markdown.string ().size ()) {
        /* if relative, assume relative to config dir */
        if (!a->signature_file_markdown.is_absolute ()) {
          a->signature_file_markdown = astroid->standard_paths ().config_dir / a->signature_file_markdown;
        }

        if (bfs::exists (a->signature_file_markdown) &&
            bfs::is_regular_file (a->signature_file_markdown))
          a->has_signature_markdown = true; // requires text signature
      }

      a->gpgkey =
        (sv = kv.second.get_optional<string> ("gpgkey"),
        (sv ? sv.get () : ""));
      if (!a->gpgkey.empty()) {
        a->has_gpg = true;
        a->always_gpg_sign =
          (bv = kv.second.get_optional<bool> ("always_gpg_sign"),
          (bv ? bv.get () : false));
      }

      a->select_query =
        (sv = kv.second.get_optional<string> ("select_query"),
        (sv ? sv.get () : ""));

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

  Account * AccountManager::get_assosciated_account (refptr<Message> msg) {
    /* look for any accounts involved in message */
    for (Address &a : msg->all_to_from().addresses) {
      if (is_me (a)) {
        LOG (debug) << "ac: found address involved in conversation: " << a.full_address ();
        return get_account_for_address (a);
      }
    }

    /* look for account with query containing message */
    if (msg->in_notmuch) {
      Db db;
      for (Account &a : accounts) {
        if (!a.select_query.empty ()) {
          if (msg->nmmsg->in_query (&db, a.select_query)) {
            LOG (debug) << "ac: found address matching query: " << a.full_address ();
            return &a;
          }
        }
      }
    }

    LOG (debug) << "ac: could not find associated address, using default.";

    /* no matching account found, use default */
    return &(accounts[default_account]);
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

