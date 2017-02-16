# pragma once

# include <functional>

# include "astroid.hh"

# include <boost/filesystem.hpp>
# include <boost/property_tree/ptree.hpp>
# include <boost/property_tree/json_parser.hpp>

namespace bfs = boost::filesystem;
using boost::property_tree::ptree;

namespace Astroid {
  struct StandardPaths {
    bfs::path home;
    bfs::path config_dir;
    bfs::path data_dir;
    bfs::path cache_dir;
    bfs::path runtime_dir;
    bfs::path config_file;
    bfs::path searches_file;
    bfs::path plugin_dir;
    bfs::path save_dir;
    bfs::path attach_dir;
  };

  struct RuntimePaths {
    bfs::path save_dir;   // last used save to folder
    bfs::path attach_dir; // last used attach from folder
  };

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

      StandardPaths std_paths;
      RuntimePaths  run_paths;


      void load_config (bool initial = false);
      void load_dirs ();
      bool check_config (ptree);
      void write_back_config ();

      ptree config;
      ptree notmuch_config;

      const int CONFIG_VERSION = 7;

    private:
      ptree setup_default_config (bool);

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
          std::function<void(const ptree &,
            const ptree::path_type &,
            const ptree&)> method);

      void merge(const ptree &parent,
                 const ptree::path_type &childPath,
                 const ptree &child);


      void merge_ptree (const ptree &pt);
  };
}

