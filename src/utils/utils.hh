# include "ustring_utils.hh"
# include "vector_utils.hh"
# include "date_utils.hh"

# include <boost/filesystem.hpp>

# pragma once

namespace bfs = boost::filesystem;

namespace Astroid {

  class Utils {
    public:

      static void init ();

      /* return human readable file size */
      static ustring format_size (int sz);

      /* make filename safe */
      static ustring safe_fname (ustring fname);

      /* expand ~ to HOME */
      static bfs::path expand (bfs::path);

      /* get tag color */
      static std::pair<ustring, ustring> get_tag_color (ustring, unsigned char canvascolor[3]);
      static float        tags_alpha;
      static Pango::Color tags_upper_color;
      static Pango::Color tags_lower_color;

  };
}

