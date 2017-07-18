# include "utils.hh"
# include "crypto.hh"

# include <string>
# include <iostream>
# include <iomanip>
# include <exception>

# include <glib.h>
# include <boost/property_tree/ptree.hpp>
# include <boost/filesystem.hpp>
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

namespace bfs = boost::filesystem;
using boost::property_tree::ptree;
using std::endl;

namespace Astroid {
  Pango::Color Utils::tags_upper_color;
  Pango::Color Utils::tags_lower_color;
  float        Utils::tags_alpha;

  void Utils::init () {
    ptree ti = astroid->config ("thread_index.cell");

    ustring _tags_upper_color = ti.get<std::string> ("tags_upper_color");
    ustring _tags_lower_color = ti.get<std::string> ("tags_lower_color");

    bool r = false;
    r = tags_upper_color.parse (_tags_upper_color);
    if (!r) {
      LOG (error) << "ti: failed parsing tags_upper_color";
      tags_upper_color.parse ("#e5e5e5");
    }

    r = tags_lower_color.parse (_tags_lower_color);
    if (!r) {
      LOG (error) << "ti: failed parsing tags_lower_color";
      tags_lower_color.parse ("#e5e5e5");
    }

    tags_alpha = ti.get<float> ("tags_alpha");
    if (tags_alpha > 1) tags_alpha = 1;
    if (tags_alpha < 0) tags_alpha = 0;
  }

  ustring Utils::format_size (int sz) {

    /* Glib::format_size is not yet likely to be available */

    gchar * ch = g_format_size (sz);
    ustring s (ch);

    g_free (ch);
    return s;
  }

  ustring Utils::safe_fname (ustring fname) {
    // allowed chars in file names
    auto pattern = Glib::Regex::create ("[^0-9A-Za-z.\\- :\\(\\)]");

    // double _
    auto pattern2 = Glib::Regex::create ("__+");

    ustring _f = fname;
    _f = pattern->replace (_f, 0, "_", static_cast<Glib::RegexMatchFlags>(0));
    _f = pattern2->replace (_f, 0, "", static_cast<Glib::RegexMatchFlags>(0));

    return _f;
  }

  bfs::path Utils::expand (bfs::path in) {
    ustring s = in.c_str ();
    if (s.size () < 1) return in;

    const char * home = getenv ("HOME");
    if (home == NULL) {
      LOG (error) << "error: HOME variable not set.";
      throw std::invalid_argument ("error: HOME environment variable not set.");
    }

    if (s[0] == '~') {
      s = ustring(home) + s.substr (1, s.size () - 1);
      return bfs::path (s);
    } else {
      return in;
    }
  }

  ustring Utils::rgba_to_hex (Gdk::RGBA c) {
    std::ostringstream str;
    str << "#";

    str << std::hex << std::setfill('0') << std::setw(2) <<
      ((int) c.get_red_u () * 255 / 65535) ;
    str << std::hex << std::setfill('0') << std::setw(2) <<
      ((int) c.get_green_u () * 255 / 65535) ;
    str << std::hex << std::setfill('0') << std::setw(2) <<
      ((int) c.get_blue_u () * 255 / 65535) ;

    str << std::hex << std::setfill('0') << std::setw(2) <<
      (int) (c.get_alpha () * 255);

    return str.str ();
  }

  std::pair<Gdk::RGBA, Gdk::RGBA> Utils::get_tag_color_rgba (ustring t, unsigned char cv[3])
  {
    # ifndef DISABLE_PLUGINS

    Gdk::RGBA canvas;
    canvas.set_red_u   ( cv[0] * 65535 / 255 );
    canvas.set_green_u ( cv[1] * 65535 / 255 );
    canvas.set_blue_u  ( cv[2] * 65535 / 255 );

    auto clrs = astroid->plugin_manager->astroid_extension->get_tag_colors (t, rgba_to_hex (canvas));

    if (!clrs.first.empty () || !clrs.second.empty ()) {
      return std::make_pair (Gdk::RGBA (clrs.first), Gdk::RGBA (clrs.second));
    }
    # endif

    unsigned char * tc = Crypto::get_md5_digest_char (t);

    unsigned char upper[3] = {
      (unsigned char) tags_upper_color.get_red (),
      (unsigned char) tags_upper_color.get_green (),
      (unsigned char) tags_upper_color.get_blue (),
      };

    unsigned char lower[3] = {
      (unsigned char) tags_lower_color.get_red (),
      (unsigned char) tags_lower_color.get_green (),
      (unsigned char) tags_lower_color.get_blue (),
      };

    /*
     * normalize the background tag color to be between upper and
     * lower, then choose light or dark font color depending on
     * luminocity of background color.
     */

    unsigned char bg[3];

    for (int k = 0; k < 3; k++) {
      bg[k] = tc[k] * (upper[k] - lower[k]) + lower[k];
    }

    Gdk::RGBA bc;
    bc.set_rgba_u ( bg[0] * (65535) / (255),
                    bg[1] * (65535) / (255),
                    bg[2] * (65535) / (255),
                    (unsigned short) (tags_alpha * (65535)));

    delete tc;

    float lum = ((bg[0] * tags_alpha + (1-tags_alpha) * cv[0] ) * .2126 + (bg[1] * tags_alpha + (1-tags_alpha) * cv[1]) * .7152 + (bg[2] * tags_alpha + (1-tags_alpha) * cv[0]) * .0722) / 255.0;
    /* float avg = (bg[0] + bg[1] + bg[2]) / (3 * 255.0); */


    Gdk::RGBA fc;
    if (lum > 0.5) {
      fc.set ("#000000");
    } else {
      fc.set ("#f2f2f2");
    }

    return std::make_pair (fc, bc);
  }

  std::pair<ustring, ustring> Utils::get_tag_color (ustring t, unsigned char cv[3]) {
    auto clrs = get_tag_color_rgba (t, cv);

    return std::make_pair (rgba_to_hex (clrs.first), rgba_to_hex (clrs.second));
  }
}

