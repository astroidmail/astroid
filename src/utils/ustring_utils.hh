# pragma once

# include <boost/tokenizer.hpp>

# include "astroid.hh"

namespace Astroid {
  class UstringUtils {
    public:
      /*
       * ustring tokenizer: http://lists.boost.org/boost-users/2007/01/24698.php
       *
       */
      typedef boost::tokenizer<
        boost::char_delimiters_separator< Glib::ustring::value_type > ,
        Glib::ustring::const_iterator ,
        Glib::ustring > utokenizer;

      static void trim (ustring &);
      static void trim_left (ustring &);
      static void trim_right (ustring &);
      static ustring random_alphanumeric (int);
      static ustring replace (ustring subject, const ustring& search,
                          const ustring& replace);

      static ustring unixify (const ustring subject);

      /* converts a byte array to a ustring */
      static std::pair<bool, ustring> data_to_ustring (unsigned int len, const char * data);
      static std::pair<bool, ustring> bytearray_to_ustring (refptr<Glib::ByteArray> &ba);
  };
}

