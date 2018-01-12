# pragma once

# include "astroid.hh"

# include <boost/filesystem.hpp>
using namespace boost::filesystem;

namespace Astroid {
  class Resource {
    public:
      static void init (const char *);

      Resource (bool has_user, const char * path);
      Resource (bool has_user, ustring path);
      Resource (bool has_user, path path);

      path get_path ();

      static path get_exe_dir ();
      static path get_cwd ();

      static const char * argv0;

    private:
      path finalpath;
  };
}

