# include "ustring_utils.hh"
# include "vector_utils.hh"
# include "date_utils.hh"

# pragma once

namespace Astroid {

  class Utils {
    public:

      /* return human readable file size */
      static ustring format_size (int sz);

  };
}

