# pragma once

# include <atomic>
# include <boost/filesystem.hpp>

# include "proto.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  class Theme {
    public:
      Theme ();

      static atomic<bool> theme_loaded;
      static const char *  thread_view_html_f;
      static const char *  thread_view_scss_f;
      static ustring       thread_view_html;
      static ustring       thread_view_css;
      const char * STYLE_NAME = "STYLE";
      const int THEME_VERSION = 2;

    private:
      bool check_theme_version (path);
      ustring process_scss (const char * scsspath);
  };
}

