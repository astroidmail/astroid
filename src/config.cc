# include <iostream>
# include <stdlib.h>

# include <boost/filesystem.hpp>

# include "config.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  Config::Config () {
    /* default config */
    char * config_home = getenv ("XDG_CONFIG_HOME");
    char * home_c      = getenv ("HOME");

    if (home_c == NULL) {
      cerr << "cf: HOME environment variable not set." << endl;
      exit (1);
    }

    home = path(home_c);

    path default_config = home / path(".config/astroid/config");

    if (config_home == NULL) {
      config_file = default_config;
    } else {
      config_file = path(config_home) / path("astroid/config");
    }

    load_config ();
  }

  Config::Config (const char * fname) {
    char * home_c      = getenv ("HOME");

    if (home_c == NULL) {
      cerr << "cf: HOME environment variable not set." << endl;
      exit (1);
    }

    home = path(home_c);

    config_file = path(fname);

    load_config ();
  }

  void Config::load_config () {
    cout << "cf: loading: " << config_file << endl;

    config_dir = absolute(config_file.parent_path());
    if (!is_directory(config_dir)) {
      cout << "cf: making config dir.." << endl;
      create_directories (config_dir);
    }

    if (!is_regular_file (config_file)) {
      cout << "cf: no config, using defaults." << endl;
      return;
    }

  }

}

