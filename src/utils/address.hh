# pragma once

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {

  class Address {
    public:
      Address ();
      Address (ustring full_address);
      Address (ustring name, ustring email);

      ustring email();
      ustring name();
      ustring fail_safe_name ();
      bool    valid ();

      ustring full_address();

    private:
      ustring _name;
      ustring _email;
      bool _valid;
  };

}


