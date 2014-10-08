# pragma once

# include "astroid.hh"
# include "proto.hh"

# include <vector>

# include <gmime/gmime.h>

using namespace std;

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

      vector<Address> addresses;
      ustring str ();

      AddressList& operator+= (Address &);
      AddressList& operator+= (AddressList &);

      void remove_me ();
  };

}


