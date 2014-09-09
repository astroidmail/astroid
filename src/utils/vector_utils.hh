# pragma once

# include "astroid.hh"
# include <vector>

using namespace std;

namespace Astroid {
  class VectorUtils {
    public:

      static vector<ustring> split_and_trim (ustring &, ustring);
      static ustring concat (vector<ustring> &, ustring, vector<ustring>);
      static ustring concat_tags (vector<ustring>);
      static ustring concat_authors (vector<ustring>);

      static const vector<ustring> stop_ons_tags;
      static const vector<ustring> stop_ons_auth;

  };
}

