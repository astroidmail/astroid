# pragma once

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {

  class MailQuotes {
    public:
      static void filter_quotes (ustring &body);
      static ustring::iterator insert_string (ustring &to, ustring::iterator &to_start, const ustring &src);

      static bool compare (ustring::iterator, ustring::const_iterator, ustring);
  };

}

