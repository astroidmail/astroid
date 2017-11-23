# include <iostream>
# include <vector>
# include <algorithm>

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

  ustring UstringUtils::random_string_from_charset (const ustring chars, const int length) {
    ustring str;
    uint32_t r;

    for (int i = 0; i < length; i++) {
      r = arc4random_uniform(chars.length());   // This is recommended over constructions
                                                 // like "arc4random() % upper_bound" as it
                                                 // avoids "modulo bias" when the upper
                                                 // bound is not a power of two.
      str += chars[r];
    }    

    return str;
  }
  
  ustring UstringUtils::random_alphanumeric (int length) {
    const ustring _chars = "abcdefghijklmnopqrstuvwxyz1234567890";
    return UstringUtils::random_string_from_charset(_chars, length);
  }

  ustring UstringUtils::random_rfc5322_atext (int length) {
    // 81 allowed charaters for the message id as per RFC 5322
    const ustring _chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'*+-/=?^_`{|}~";
    return UstringUtils::random_string_from_charset(_chars, length);
  }

  ustring UstringUtils::random_file_name (const ustring suffix) {
    return (UstringUtils::random_alphanumeric (50)   // use at least 256 bit of
                                                     // entropy to remain sufficiently
                                                     // unique for the forseeable future
	    + suffix);
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
    std::string  u;
    bool success;

    std::string in (data, len);

    try {
      /* this assumes that the input is in UTF-8 rather than trying to detect the input encoding.
       * UTF-16 is a superset of UTF-8 so we convert around UTF-16 to remove any invalid characters. */
      u = Glib::convert_with_fallback (in, "UTF-16", "UTF-8");
      u = Glib::convert_with_fallback (u,  "UTF-8", "UTF-16");
      success = true;
    } catch (Glib::ConvertError &ex) {
      LOG (error) << "ustring: could not convert data: " << ex.what ();
      success = false;
    }

    return std::make_pair (success, u);
  }

  std::pair<bool, ustring> UstringUtils::bytearray_to_ustring (refptr<Glib::ByteArray> & ba) {
    gchar *      in   = (gchar *) ba->get_data ();
    unsigned int len  = ba->size ();

    return data_to_ustring (len, in);
  }
}

