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

      path config_file;
      path config_dir;

      void load_config ();

  };
}

