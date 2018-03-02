# pragma once

# include <glibmm.h>

namespace Astroid {
  class UstringUtils {
    public:
      static void trim (Glib::ustring &);
      static void trim_left (Glib::ustring &);
      static void trim_right (Glib::ustring &);
      static Glib::ustring random_alphanumeric (int);
      static Glib::ustring replace (Glib::ustring subject, const Glib::ustring& search,
                          const Glib::ustring& replace);

      static Glib::ustring unixify (const Glib::ustring subject);

      /* converts a byte array to a ustring */
      static std::pair<bool, Glib::ustring> data_to_ustring (unsigned int len, const char * data);
      static std::pair<bool, Glib::ustring> bytearray_to_ustring (Glib::RefPtr<Glib::ByteArray> &ba);
  };
}

