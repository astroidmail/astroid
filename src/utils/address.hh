# pragma once

# include "astroid.hh"
# include "proto.hh"

# include <vector>

# include <gmime/gmime.h>

namespace Astroid {

  /* Encoding and quoting:
   *
   * On construction of Address and AddressLists the full email address (mbox)
   * gets interpreted (decoded and unquoted) by gmime. It is saved in UTF-8 in
   * an ustring.
   *
   * When returnd from Address::full_address() or AddressList::str() it is not
   * re-encoded, but it remains quoted. It is therefore suitable for an UTF-8
   * editor which saves the draft email with the email addresses in quoted,
   * unencoded, format.
   *
   * ComposeMessage loads the draft email (without decoding the address) and
   * before displaying, sending or saving it build()'s and finalize()'s the message
   * encoding the quoted address.
   *
   */

  class Address {
    public:
      Address ();
      Address (ustring full_address);
      Address (ustring name, ustring email);
      Address (InternetAddress *);

      ustring email();
      ustring name();
      ustring fail_safe_name ();
      bool    valid ();

      ustring full_address ();
      InternetAddress * get_iaddr ();

    private:
      ustring _name   = "";
      ustring _email  = "";
      bool _valid     = false;
  };

  class AddressList {
    public:
      AddressList ();
      AddressList (ustring);
      AddressList (InternetAddressList *);
      AddressList (Address);

      std::vector<Address> addresses;
      ustring str ();
      int size ();
      bool empty ();

      AddressList& operator=  (const AddressList &);
      AddressList& operator+= (const Address &);
      AddressList& operator+= (const AddressList &);
      AddressList  operator+  (const Address &) const;
      AddressList  operator+  (const AddressList &) const;

      AddressList  operator-  (const AddressList &);
      AddressList& operator-= (const AddressList &);

      void remove_me ();
      void remove_duplicates ();
  };

}


