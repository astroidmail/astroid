# pragma once

# include "astroid.hh"
# include <vector>

namespace Astroid {
  template<class T> bool has (std::vector<T> v, T e) {
    return (find(v.begin (), v.end (), e) != v.end ());
  }

  class VectorUtils {
    public:

      static std::vector<ustring> split_and_trim (ustring & str, ustring delim);
      static ustring concat (std::vector<ustring> &, ustring, std::vector<ustring>);
      static ustring concat_tags (std::vector<ustring>);
      static ustring concat_authors (std::vector<ustring>);

      static const std::vector<ustring> stop_ons_tags;
      static const std::vector<ustring> stop_ons_auth;

  };
}

