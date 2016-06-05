# include <iostream>
# include <vector>

# include "astroid.hh"
# include "vector_utils.hh"
# include "ustring_utils.hh"
# include "utils.hh"

using namespace std;

namespace Astroid {

  const vector<ustring> VectorUtils::stop_ons_tags = {" "};

  vector<ustring> VectorUtils::split_and_trim (const ustring &str, const ustring delim) {

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

  ustring VectorUtils::concat_tags_color (
      vector<ustring> tags,
      bool pango,
      int maxlen ) {

    ustring tag_string = "";
    bool first = true;
    bool broken = false;
    int len = 0;

    for (auto t : tags) {

      if (!first) {
        tag_string += " ";
      } else first = false;

      auto colors = Utils::get_tag_color (t);

      if (maxlen > 0) {
        broken = true;
        if (len >= maxlen) break;
        broken = false;

        if ((len + t.length () + 2) > static_cast<unsigned int>(maxlen)) {
          t = t.substr (0, (len + t.length () + 2 - maxlen));
          t += "..";
        }

        len += t.length () + 2;
      }

      if (pango) {
        tag_string += ustring::compose (
                    "<span bgcolor=\"%3\" color=\"%1\"> %2 </span>",
                    colors.first,
                    Glib::Markup::escape_text(t),
                    colors.second );

      } else {
        tag_string += ustring::compose (
                    "<span style=\"background-color: %3; color: %1 !important; white-space: pre;\"> %2 </span>",
                    colors.first,
                    Glib::Markup::escape_text(t),
                    colors.second );
      }
    }

    if (broken) {
      tag_string += "..";
    }


    return tag_string;
  }
}

