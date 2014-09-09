# include "address.hh"
# include "astroid.hh"
# include "log.hh"

# include <gmime/gmime.h>

# include "ustring_utils.hh"

using namespace std;

namespace Astroid {
  Address::Address () {
    _valid = false;
  }

  Address::Address (ustring full_address) {
    /* parse and split */
    InternetAddressList * list =
      internet_address_list_parse_string (full_address.c_str());

    if (internet_address_list_length (list) > 1) {
      log << error << "address: more than one address in list!" << endl;
      _valid = false;
      _email = full_address;
      g_object_unref (list);
      return;
    }

    if (internet_address_list_length (list) < 1) {
      log << error << "address: no address in string." << endl;
      _email = full_address;
      _valid = false;
      g_object_unref (list);
      return;
    }

    InternetAddress * address = internet_address_list_get_address (
        list, 0);
    if (address == NULL) {
      log << error << "address: no address in string." << endl;
      _email = full_address;
      _valid = false;
      g_object_unref (list);
      return;
    }

    InternetAddressMailbox * mbox = INTERNET_ADDRESS_MAILBOX (address);
    const char * n = internet_address_get_name (address);
    if (n != NULL)
      _name  = ustring (n);
    n = internet_address_mailbox_get_addr (mbox);
    if (n != NULL)
      _email = ustring (n);

    g_object_unref (list);
  }

  Address::Address (ustring __name, ustring __email) :
    _name (__name), _email (__email), _valid (true)
  {}

  bool Address::valid () {
    return _valid;
  }

  ustring Address::email () {
    return _email;
  }

  ustring Address::name () {
    return _name;
  }

  ustring Address::fail_safe_name () {
    UstringUtils::trim (_name);
    if (_name.length () == 0) {
      return _email;
    } else {
      return _name;
    }
  }

  ustring Address::full_address () {
    InternetAddress * mbox = internet_address_mailbox_new (_name.c_str(), _email.c_str());
    const char * faddr = internet_address_to_string (mbox, true);
    g_object_unref (mbox);
    return ustring(faddr);
  }


}

