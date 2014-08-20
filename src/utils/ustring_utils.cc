# include <iostream>
# include <vector>
# include <algorithm>

# include "astroid.hh"
# include "ustring_utils.hh"
# include "vector_utils.hh"

using namespace std;

namespace Astroid {
  // from https://github.com/markoa/gtkmm-utils/blob/master/glibmm-utils/ustring.cc
  void UstringUtils::trim_left(Glib::ustring& str)
  {
      if (str.empty ()) return;

      Glib::ustring::iterator it(str.begin());
      Glib::ustring::iterator end(str.end());

      for ( ; it != end; ++it)
          if (! isspace(*it))
              break;

      if (it == str.end())
          str.clear();
      else
          str.erase(str.begin(), it);
  }

  void UstringUtils::trim_right(Glib::ustring& str)
  {
      if (str.empty ()) return;

      Glib::ustring::iterator end(str.end());
      Glib::ustring::iterator it(--(str.end()));

      for ( ; ; --it) {
          if (! isspace(*it)) {
              Glib::ustring::iterator it_adv(it);
              str.erase(++it_adv, str.end());
              break;
          }

          if (it == str.begin()) {
              str.clear();
              break;
          }
      }
  }

  void UstringUtils::trim(Glib::ustring& str)
  {
      trim_left(str);
      trim_right(str);
  }

  ustring UstringUtils::random_alphanumeric (int length) {
    ustring str;

    const string _chars = "abcdefghijklmnopqrstuvwxyz1234567890";
    random_device rd;
    mt19937 g(rd());

    for (int i = 0; i < length; i++)
      str += _chars[g() % _chars.size()];

    return str;
  }
}

