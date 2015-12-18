# pragma once

# include <functional>

# include "astroid.hh"

# include <boost/filesystem.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>

using namespace std;
using namespace boost::filesystem;
using boost::property_tree::ptree;

namespace Astroid {
  class Config {
    public:
      Config (bool _test = false, bool no_load = false);
      Config (const char *, bool no_load = false);

      /* dir env vars from XDG with defaults:
       * XDG_CONFIG_HOME  : $HOME/.config/
       * XDG_DATA_HOME    : $HOME/.local/share/
       * XDG_CACHE_HOME   : $HOME/.cache/
       * XDG_RUNTIME_HOME : none
       */

      bool test;

      path home;
      path config_dir;
      path data_dir;
      path cache_dir;
      path runtime_dir;

      path config_file;

      void load_config (bool initial = false);
      void load_dirs ();
      void setup_default_config (bool);
      bool check_config (ptree);
      void write_back_config ();

      ptree default_config;
      ptree config;

      const int CONFIG_VERSION = 1;

    private:
      /* TODO: split into utils/ somewhere.. */
      /* merge of property trees */

      // from http://stackoverflow.com/questions/8154107/how-do-i-merge-update-a-boostproperty-treeptree
      template<typename T>
        void traverse_recursive(
            const boost::property_tree::ptree &parent,
            const boost::property_tree::ptree::path_type &childPath,
            const boost::property_tree::ptree &child, T &method);

      void traverse(
          const boost::property_tree::ptree &parent,
          function<void(const ptree &,
            const ptree::path_type &,
            const ptree&)> method);

      void merge(const ptree &parent,
                 const ptree::path_type &childPath,
                 const ptree &child);


      void merge_ptree (const ptree &pt);
  };
}

