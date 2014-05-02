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

  void Config::setup_default_config (bool initial) {
    default_config.put ("astroid.config.version", CONFIG_VERSION);  // TODO: unused
    default_config.put ("astroid.notmuch.db", "~/.mail");           // TODO: unused

    if (initial) {
      /* account - only set if no other accounts, we accomplish that
       * by only defining the default if there is no config file present.
       *
       * AccountManager will complain if there are no accounts defined.
       */
      default_config.put ("accounts.charlie.name", "Charlie Root");
      default_config.put ("accounts.charlie.email", "root@localhost");
      default_config.put ("accounts.charlie.gpgkey", "");
      default_config.put ("accounts.charlie.sendmail", "msmtp -t");
      default_config.put ("accounts.charlie.default", true);
    }

    /* ui behaviour */
    default_config.put ("thread_index.open_default", "paned");    // TODO: unused

    /* editor */
    default_config.put ("editor.gvim.double_esc_deactivates", true);
    default_config.put ("editor.gvim.default_cmd_on_enter", "i");
    default_config.put ("editor.gvim.cmd", "gvim");
    default_config.put ("editor.gvim.args", "-f -c 'set ft=mail' '+/^\\s*\\n/' '+nohl'");

  }

  void Config::write_back_config () {
    cout << "cf: writing back config to: " << config_file << endl;

    write_json (config_file.c_str (), config);
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
      setup_default_config (true);
      config = default_config;
      write_back_config ();
    } else {
      ptree new_config;
      setup_default_config (false);
      config = default_config;
      read_json (config_file.c_str(), new_config);

      merge_ptree (new_config);

      if (new_config != config) {
        cout << "cf: missing values in config have been updated with defaults." << endl;
        write_back_config ();
      }
    }
  }

  /* TODO: split into utils/ somewhere.. */
  /* merge of property trees */

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

