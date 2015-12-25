# pragma once

# include "astroid.hh"
# include <vector>

using namespace std;

namespace Astroid {
  template<class T> bool has (vector<T> v, T e) {
    return (find(v.begin (), v.end (), e) != v.end ());
  }

  class VectorUtils {
    public:

      static vector<ustring> split_and_trim (ustring & str, ustring delim);
      static ustring concat (vector<ustring> &, ustring, vector<ustring>);
      static ustring concat_tags (vector<ustring>);
      static ustring concat_authors (vector<ustring>);

      static const vector<ustring> stop_ons_tags;
      static const vector<ustring> stop_ons_auth;

  };
}

