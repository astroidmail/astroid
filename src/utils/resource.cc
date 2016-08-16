# include "resource.hh"
# include "build_config.hh"
# include "astroid.hh"
# include "config.hh"

# include <boost/filesystem.hpp>

using namespace boost::filesystem;
using std::endl;

namespace Astroid {
  Resource::Resource (bool has_user, path def) {
    path prefix = path(PREFIX) / path ("share/astroid");

    path local_p = def;
    char * adir = getenv ("ASTROID_DIR");
    if (adir != NULL) {
      local_p = path(adir) / def;
    }

    path prefix_p = prefix / def;

    /* if this resource is user-configurable, check there first */
    path user_p = astroid->standard_paths ().config_dir / def;

    if (astroid->in_test ()) {
      if (!exists (local_p)) {
        LOG (warn) << "re: could not find local resource: " << local_p.c_str ();
        exit (1);
      }
      finalpath = local_p;
      return;
    }

    if (has_user) {
      if (exists (user_p)) {
        LOG (info) << "re: using user configured resource: " << absolute(local_p).c_str ();
        finalpath = user_p;
        return;
      }
    }

# ifdef DEBUG
    if (exists (local_p)) {
      LOG (info) << "re: using local resource: " << absolute (local_p).c_str ();
      finalpath = local_p;
    } else {
      if (exists (prefix_p)) {
        LOG (info) << "re: using installed resource: " << absolute (prefix_p).c_str ();
        finalpath = prefix_p;
      } else {
        LOG (error) << "re: could not find resource: " << local_p.c_str () << " or " << prefix_p.c_str ();
        exit (1);
      }
    }
# else
    /* if not DEBUG, only check for installed resource */
    if (exists (prefix_p)) {
      LOG (info) << "re: using installed resource: " << absolute (prefix_p).c_str ();
      finalpath = prefix_p;
    } else {
      LOG (error) << "re: could not find resource: " << prefix_p.c_str ();
      exit (1);
    }
# endif
  }

  Resource::Resource (bool hu, ustring p) : Resource (hu, path(p)) { }
  Resource::Resource (bool hu, const char * p) : Resource (hu, path(p)) { }

  path Resource::get_path () {
    return finalpath;
  }
}

