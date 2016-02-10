# pragma once

# include "astroid.hh"

# include <boost/filesystem.hpp>
using namespace boost::filesystem;

namespace Astroid {
  class Resource {
    public:
      Resource (bool has_user, const char * path);
      Resource (bool has_user, ustring path);
      Resource (bool has_user, path path);

      path get_path ();

    private:
      path finalpath;
  };
}

