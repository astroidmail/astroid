# pragma once

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {

  class MailQuotes {
    public:
      static const bool mq_verbose = false;
      static const bool delete_quote_markers = true;
      static void filter_quotes (ustring &body);
  };

}

