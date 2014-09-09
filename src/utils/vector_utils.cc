# include <iostream>
# include <vector>

# include "astroid.hh"
# include "vector_utils.hh"
# include "ustring_utils.hh"

using namespace std;

namespace Astroid {

  const vector<ustring> VectorUtils::stop_ons_tags = {" "};
  const vector<ustring> VectorUtils::stop_ons_auth = {" ", "@"};

  vector<ustring> VectorUtils::split_and_trim (ustring &str, ustring delim) {

    vector<ustring> parts = Glib::Regex::split_simple(delim, str);

    /* trim */
    for_each (parts.begin (),
              parts.end (),
              [&](ustring &a) {

                UstringUtils::trim(a);
              });

    return parts;
  }

  ustring VectorUtils::concat (vector<ustring> &strs, ustring delim, vector<ustring> stop_on) {
    /* stop_on is a list of strings which each element be trunctated
     * to if it exists in the element */

    bool first = true;
    ustring out;

    for_each (strs.begin (),
              strs.end (),
              [&](ustring a) {

                if (!first) {
                  out += delim;
                } else {
                  first = false;
                }

                for_each (stop_on.begin (),
                          stop_on.end (),
                          [&](ustring s) {
                            a = a.substr(0,a.find_first_of(s));
                          });
                out += a;
              });

    return out;
  }

  ustring VectorUtils::concat_tags (vector<ustring> tags) {
    return concat (tags, ", ", stop_ons_tags);
  }

  ustring VectorUtils::concat_authors (vector<ustring> auths) {
    return concat (auths, ",", stop_ons_auth);
  }
}

