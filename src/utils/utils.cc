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

}

