# include "address.hh"
# include "astroid.hh"

# include <gmime/gmime.h>

# include "ustring_utils.hh"
# include "address.hh"
# include "account_manager.hh"

using namespace std;

namespace Astroid {
  Address::Address () {
    _valid = false;
  }

  Address::Address (ustring full_address) {
    /* parse and split */
    InternetAddressList * list =
      internet_address_list_parse_string (full_address.c_str());

    _valid = true;

    if (internet_address_list_length (list) > 1) {
      LOG (error) << "address: more than one address in list!";
      _valid = false;
      _email = full_address;
      g_object_unref (list);
      return;
    }

    if (internet_address_list_length (list) < 1) {
      LOG (error) << "address: no address in string.";
      _email = full_address;
      _valid = false;
      g_object_unref (list);
      return;
    }

    InternetAddress * address = internet_address_list_get_address (
        list, 0);
    if (address == NULL) {
      LOG (error) << "address: no address in string.";
      _email = full_address;
      _valid = false;
      g_object_unref (list);
      return;
    }

    InternetAddressMailbox * mbox = INTERNET_ADDRESS_MAILBOX (address);
    const char * n = internet_address_get_name (address);
    if (n != NULL)
      _name  = ustring (g_mime_utils_header_decode_text(n));
    n = internet_address_mailbox_get_addr (mbox);
    if (n != NULL)
      _email = ustring (g_mime_utils_header_decode_text(n));

    g_object_unref (list);
  }

  Address::Address (InternetAddress * addr) {
    InternetAddressMailbox * mbox = INTERNET_ADDRESS_MAILBOX (addr);
    const char * n = internet_address_get_name (addr);
    if (n != NULL)
      _name  = ustring (g_mime_utils_header_decode_text(n));
    n = internet_address_mailbox_get_addr (mbox);
    if (n != NULL)
      _email = ustring (g_mime_utils_header_decode_text(n));

    _valid = true;
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
    const char * faddr = internet_address_to_string (mbox, false);
    g_object_unref (mbox);
    return ustring(faddr);
  }

  InternetAddress * Address::get_iaddr () {
    InternetAddress * mbox = internet_address_mailbox_new (_name.c_str(), _email.c_str());

    // now owned by caller

    return mbox;
  }

  AddressList::AddressList () {

  }

  AddressList::AddressList (InternetAddressList * list) {
    for (int i = 0; i < internet_address_list_length (list); i++) {
      InternetAddress * a = internet_address_list_get_address (list, i);
      addresses.push_back (Address (a));
    }
  }

  AddressList::AddressList (ustring _str)
  {
    if (!_str.empty ())  {
      InternetAddressList * list = internet_address_list_parse_string (_str.c_str ());

      for (int i = 0; i < internet_address_list_length (list); i++) {
        InternetAddress * a = internet_address_list_get_address (list, i);
        addresses.push_back (Address (a));
      }
    }
  }

  AddressList::AddressList (Address a) {
    addresses.push_back (a);
  }

  ustring AddressList::str () {
    InternetAddressList * list = internet_address_list_new ();
    for (Address &a : addresses) {
      InternetAddress * addr = a.get_iaddr ();
      internet_address_list_add (list, addr);
      g_object_unref (addr);
    }

    const char * addrs = internet_address_list_to_string (list, false);

    ustring r ("");

    if (addrs != NULL) r = ustring (g_mime_utils_header_decode_text(addrs));

    g_object_unref (list);

    return r;
  }

  AddressList& AddressList::operator+= (const Address & a) {
    addresses.push_back (a);

    return *this;
  }

  AddressList& AddressList::operator+= (const AddressList & al) {
    for (const Address &a : al.addresses) {
      addresses.push_back (a);
    }

    return *this;
  }

  AddressList AddressList::operator+ (const Address &b) const {
    AddressList a;
    a += *this;
    a.addresses.push_back (b);

    return a;
  }

  AddressList AddressList::operator+ (const AddressList &b) const {
    AddressList a;
    a += *this;
    a += b;

    return a;
  }

  AddressList& AddressList::operator= (const AddressList &b) {
    addresses.clear ();
    addresses = b.addresses;

    return *this;
  }

  AddressList AddressList::operator- (const AddressList &b) {
    AddressList a;
    AddressList bb = b;

    std::sort (addresses.begin (), addresses.end (), [](Address i, Address j) { return i.email () < j.email (); });
    std::sort (bb.addresses.begin (), bb.addresses.end (), [](Address i, Address j) { return i.email () < j.email (); });

    set_difference (addresses.begin (), addresses.end (),
                    bb.addresses.begin (), bb.addresses.end (),
                    std::inserter (a.addresses, a.addresses.begin ()),
                    [](Address i, Address j) { return i.email () < j.email (); });

    return a;
  }

  AddressList& AddressList::operator-= (const AddressList &b) {
    AddressList a = *this - b;
    *this = a;

    return *this;
  }

  int AddressList::size () {
    return addresses.size ();
  }

  bool AddressList::empty () {
    return addresses.empty ();
  }

  void AddressList::remove_me () {

    vector<Address>::iterator it;
    while (it = find_if  (addresses.begin (),
                          addresses.end (),
                          [&](Address &a) {
                            return astroid->accounts->is_me (a);
                          }),
           it != addresses.end () )
    {

      addresses.erase (it);

    }

  }

  void AddressList::remove_duplicates () {
    std::sort (addresses.begin (), addresses.end (), [](Address i, Address j) { return i.email () < j.email (); });
    addresses.erase (std::unique (addresses.begin (), addresses.end (), [](Address i, Address j) { return i.email () == j.email (); }), addresses.end ());
  }
}

