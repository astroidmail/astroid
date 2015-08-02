# include "utils.hh"
# include <glib.h>

namespace Astroid {

  ustring Utils::format_size (int sz) {

    /* Glib::format_size is not yet likely to be available */

    gchar * ch = g_format_size (sz);
    ustring s (ch);

    g_free (ch);
    return s;
  }

  ustring Utils::safe_fname (ustring fname) {
    auto pattern = Glib::Regex::create ("[^0-9A-Za-z.\\- :]"); // chars allowed
    auto pattern2 = Glib::Regex::create ("__+"); // double _

    ustring _f = fname;
    _f = pattern->replace (_f, 0, "_", static_cast<Glib::RegexMatchFlags>(0));
    _f = pattern2->replace (_f, 0, "", static_cast<Glib::RegexMatchFlags>(0));

    return _f;
  }

}

