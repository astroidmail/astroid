# include <iostream>
# include <vector>
# include <algorithm>
# include <random>

# include "astroid.hh"
# include "proto.hh"
# include "ustring_utils.hh"
# include "vector_utils.hh"

using namespace std;

namespace Astroid {
  // from https://github.com/markoa/gtkmm-utils/blob/master/glibmm-utils/ustring.cc
  void UstringUtils::trim_left(Glib::ustring& str)
  {
      if (str.empty ()) return;

      Glib::ustring::iterator it  (str.begin());
      Glib::ustring::iterator end (str.end());

      for ( ; it != end; ++it)
          if (! g_unichar_isspace(*it))
              break;

      if (it == end)
          str.clear();
      else
          str.erase(str.begin(), it);
  }

  void UstringUtils::trim_right(Glib::ustring& str)
  {
      if (str.empty ()) return;

      Glib::ustring::reverse_iterator rit  (str.rbegin());
      Glib::ustring::reverse_iterator rend (str.rend());

      for ( ; rit != rend; ++rit)
        if (! g_unichar_isspace (*rit))
          break;

      if (rit == rend)
        str.clear ();
      else
        str.erase (rit.base (), str.end ());

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

  /* http://stackoverflow.com/a/15372760/377927 */
  ustring UstringUtils::replace (ustring subject, const ustring& search,
                          const ustring& replace) {
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;
  }

  ustring UstringUtils::unixify (const ustring subject) {
    /* replace CRs with newlines */
    ustring s = replace (subject, "\r\n", "\n");

    return s;
  }

  std::pair<bool, ustring> UstringUtils::data_to_ustring (unsigned int len, const char * data) {
    stringstream s;
    bool         success;

    /* convert */
    gsize read, written;
    GError * err = NULL;
    gchar * out = g_convert_with_fallback (data, len, "UTF-8", "ASCII", NULL,
                                           &read, &written, &err);
    if (out != NULL) {
      s.write (out, written);
      success = true;
    } else {
      LOG (error) << "ustring: could not convert: " << data;
      success = false;
    }

    g_free (out);

    ustring u (s.str ());

    return std::make_pair (success, u);
  }

  std::pair<bool, ustring> UstringUtils::bytearray_to_ustring (refptr<Glib::ByteArray> & ba) {
    gchar *      in   = (gchar *) ba->get_data ();
    unsigned int len  = ba->size ();

    return data_to_ustring (len, in);
  }
}

