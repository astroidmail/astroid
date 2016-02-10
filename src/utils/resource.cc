# include "resource.hh"
# include "build_config.hh"
# include "config.hh"
# include "log.hh"

# include <boost/filesystem.hpp>

using namespace boost::filesystem;
using std::endl;

namespace Astroid {
  Resource::Resource (bool has_user, path def) {
    path prefix = path(PREFIX) / path ("share/astroid");

    path local_p = def;
    path prefix_p = prefix / local_p;

    /* if this resource is user-configurable, check there first */
    path user_p = astroid->standard_paths ().config_dir / local_p;

    if (has_user) {
      if (exists (user_p)) {
        log << info << "re: using user configured resource: " << absolute(local_p).c_str () << endl;
        finalpath = user_p;
        return;
      }
    }

# ifdef DEBUG
    if (exists (local_p)) {
      log << info << "re: using local resource: " << absolute (local_p).c_str () << endl;
      finalpath = local_p;
    } else {
      if (exists (prefix_p)) {
        log << info << "re: using installed resource: " << absolute (prefix_p).c_str () << endl;
        finalpath = prefix_p;
      } else {
        log << error << "re: could not find resource: " << local_p.c_str () << " or " << prefix_p.c_str () << endl;
        exit (1);
      }
    }
# else
    /* if not DEBUG, only check for installed resource */
    if (exists (prefix_p)) {
      log << info << "re: using installed resource: " << absolute (prefix_p).c_str () << endl;
      finalpath = prefix_p;
    } else {
      log << error << "re: could not find resource: " << local_p.c_str () << " or " << prefix_p.c_str () << endl;
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

