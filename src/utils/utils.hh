# include "ustring_utils.hh"
# include "vector_utils.hh"
# include "date_utils.hh"

# pragma once

namespace Astroid {

  class Utils {
    public:

      static void init ();

      /* return human readable file size */
      static ustring format_size (int sz);

      /* make filename safe */
      static ustring safe_fname (ustring fname);

      /* get tag color */
      static std::pair<ustring, ustring> get_tag_color (ustring);
      static Pango::Color tags_upper_color;
      static Pango::Color tags_lower_color;

  };
}

