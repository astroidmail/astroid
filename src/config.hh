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
      Config ();
      Config (const char *);

      path home;
      path config_file;
      path config_dir;

      void load_config ();
      void setup_default_config ();
      void write_back_config ();

      ptree default_config;
      ptree config;

    private:
      /* merging of the property trees */

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

