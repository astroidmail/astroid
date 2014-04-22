# pragma once

# include "astroid.hh"

# include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  class Config {
    public:
      Config ();
      Config (const char *);

      path home;
      path config_file;
      path config_dir;

      void load_config ();

      /* replace with a boost property tree
      const char * default_config =
        "[astroid]\n"
        "db=~/.mail\n"
        "\n"
        "[account 0]\n"
        "name=\n"
        "email=\n"
        "gpgkey=\n"
        "\n"
        "[thread_index]\n"
        "open_default=paned\n"
        "\n";
        */

  };
}

