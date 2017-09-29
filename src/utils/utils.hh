# include "ustring_utils.hh"
# include "vector_utils.hh"
# include "date_utils.hh"

# include <boost/filesystem.hpp>
# include <boost/property_tree/ptree.hpp>

# pragma once

namespace bfs = boost::filesystem;
using boost::property_tree::ptree;

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
      static std::pair<Gdk::RGBA, Gdk::RGBA> get_tag_color_rgba (ustring, guint8 canvascolor[3]);
      static std::pair<ustring, ustring> get_tag_color (ustring, guint8 canvascolor[3]);
      static ustring      rgba_to_hex (Gdk::RGBA);
      static float        tags_alpha;
      static Pango::Color tags_upper_color;
      static Pango::Color tags_lower_color;

      /* property tree */
      static void extend_ptree (ptree &p, ptree &v) {
        p.push_back (std::make_pair ("", v));
      }

      template <class T>
      static void extend_ptree (ptree &p, T v) {
        ptree a;
        a.put ("", v);

        p.push_back (std::make_pair ("", a));
      }
  };
}

