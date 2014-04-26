# include <iostream>
# include <stdlib.h>
# include <functional>

# include <boost/filesystem.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>

# include "config.hh"

using namespace std;
using namespace boost::filesystem;
using boost::property_tree::ptree;

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

  void Config::setup_default_config () {
    default_config.put ("astroid.db", "~/.mail");
  }

  void Config::write_back_config () {
    cout << "cf: writing back config to: " << config_file << endl;

    write_json (config_file.c_str (), config);
  }

  void Config::load_config () {
    cout << "cf: loading: " << config_file << endl;

    setup_default_config ();

    config_dir = absolute(config_file.parent_path());
    if (!is_directory(config_dir)) {
      cout << "cf: making config dir.." << endl;
      create_directories (config_dir);
    }

    config = default_config;

    if (!is_regular_file (config_file)) {
      cout << "cf: no config, using defaults." << endl;
      write_back_config ();
    } else {
      ptree new_config;
      read_json (config_file.c_str(), new_config);

      merge_ptree (new_config);
    }

  }

  /* merging of the property trees */

  // from http://stackoverflow.com/questions/8154107/how-do-i-merge-update-a-boostproperty-treeptree
  template<typename T>
    void Config::traverse_recursive(
        const boost::property_tree::ptree &parent,
        const boost::property_tree::ptree::path_type &childPath,
        const boost::property_tree::ptree &child, T &method)
    {
      using boost::property_tree::ptree;

      method(parent, childPath, child);
      for(ptree::const_iterator it=child.begin();
          it!=child.end();
          ++it) {
        ptree::path_type curPath = childPath / ptree::path_type(it->first);
        traverse_recursive(parent, curPath, it->second, method);
      }
    }

  void Config::traverse(const ptree &parent,
            function<void(const ptree &,
            const ptree::path_type &,
            const ptree&)> method)
    {
      traverse_recursive(parent, "", parent, method);
    }

  void Config::merge(const ptree &parent,
             const ptree::path_type &childPath,
             const ptree &child) {

    // overwrites existing default values
    config.put(childPath, child.data());
  }

  void Config::merge_ptree(const ptree &pt)   {
    function<void(const ptree &,
      const ptree::path_type &,
      const ptree&)> method = bind (&Config::merge, this, _1, _2, _3);

    traverse(pt, method);
  }
}

