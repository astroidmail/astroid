# pragma once

# include "astroid.hh"
# include "proto.hh"

# include <vector>

# include <gmime/gmime.h>

namespace Astroid {

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
      ustring _name;
      ustring _email;
      bool _valid;
  };

  class AddressList {
    public:
      AddressList ();
      AddressList (InternetAddressList *);
      AddressList (Address);
      AddressList (ustring);

      std::vector<Address> addresses;
      ustring str ();

      AddressList& operator+= (const Address &);
      AddressList& operator+= (const AddressList &);
      AddressList  operator+ (const Address &) const;
      AddressList  operator+ (const AddressList &) const;

      void remove_me ();
  };

}


