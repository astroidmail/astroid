# pragma once

# include <iostream>
# include <vector>

# include "astroid.hh"
# include "proto.hh"
# include "config.hh"

using namespace std;

namespace Astroid {
  class Contacts {
    public:
      Contacts ();
      ~Contacts ();

      ptree contacts_config;

      bool enable_lbdb;
      string lbdb_cmd;

      int  recent_contacts_load;
      string recent_query;
      void load_recent_contacts (int);

      void add_contact (ustring);

      vector<ustring> contacts;
  };
}

