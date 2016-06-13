# include <libpeas/peas.h>
# include <glibmm.h>
# include <vector>
# include <cstdlib>

# include <boost/filesystem.hpp>

# include "manager.hh"
# include "astroid.hh"
# include "config.hh"
# include "log.hh"
# include "build_config.hh"
# include "utils/vector_utils.hh"

# include "astroid_activatable.h"
# include "thread_index_activatable.h"

using std::endl;
namespace bfs = boost::filesystem;

/* remember to set GI_TYPELIB_PATH=$(pwd) when testing */

namespace Astroid {
  PluginManager::PluginManager (bool _disabled, bool _test) {
    log << info << "plugins: starting manager.." << endl;


    disabled = _disabled;
    test     = _test;

    if (disabled) {
      log << info << "plugins: disabled." << endl;
      return;
    }

    engine = peas_engine_get_default ();
    peas_engine_enable_loader (engine, "python3");

    bfs::path plugin_dir = astroid->standard_paths().plugin_dir;

    /* add installed location */
    if (!test) {
      bfs::path prefix_p (PREFIX);
      prefix_p = prefix_p / bfs::path ("share/astroid/plugins");
      log << debug << "plugins: adding path: " << prefix_p.c_str () << endl;
      peas_engine_prepend_search_path (engine, prefix_p.c_str (), NULL);
    }

    /* adding local plugins */
    log << debug << "plugin: adding path: " << plugin_dir.c_str () << endl;
    peas_engine_prepend_search_path (engine, plugin_dir.c_str (), NULL);

    astroid_extensions = peas_extension_set_new (engine, ASTROID_TYPE_ACTIVATABLE, NULL);
    thread_index_extensions = peas_extension_set_new (engine, ASTROID_THREADINDEX_TYPE_ACTIVATABLE, NULL);

    refresh ();
  }

  PluginManager::~PluginManager () {
    if (!disabled) {
      log << debug << "plugins: uninit." << endl;
    }
  }

  void PluginManager::refresh () {
    if (disabled) return;

    log << debug << "plugins: refreshing.." << endl;
    peas_engine_rescan_plugins (engine);

    const GList * ps = peas_engine_get_plugin_list (engine);

    log << debug << "plugins: found " << g_list_length ((GList *) ps) << " plugins." << endl;

    for (; ps != NULL; ps = ps->next) {
      auto p = (PeasPluginInfo *) ps->data;

      log << debug << "plugins: loading: " << peas_plugin_info_get_name (p) << endl;

      bool e = peas_engine_load_plugin (engine, p);

      if (e) {
        log << debug << "plugins: loaded: " << peas_plugin_info_get_name (p) << endl;

        PeasExtension * pe = peas_extension_set_get_extension (astroid_extensions, p);

        /* start all astroidactivatable plugins directly */
        if (ASTROID_IS_ACTIVATABLE (pe)) {

          log << debug << "plugins: activating main plugins.." << endl;
          astroid_activatable_activate (ASTROID_ACTIVATABLE(pe));

          char buf[120] = "hello test";
          const char * k = astroid_activatable_ask (ASTROID_ACTIVATABLE (pe), buf);
          log << debug << "got: " << k << endl;

          astroid_plugins.push_back (p);
        }

        pe = peas_extension_set_get_extension (thread_index_extensions, p);

        if (ASTROID_IS_THREADINDEX_ACTIVATABLE( pe)) {
          log << debug << "plugins: activating threadindex plugin.." << endl;
          astroid_threadindex_activatable_activate (ASTROID_THREADINDEX_ACTIVATABLE(pe));

          thread_index_plugins.push_back (p);
        }

      } else {
        log << error << "plugins: failed loading: " << peas_plugin_info_get_name (p) << endl;
      }
    }
  }

  bool PluginManager::thread_index_format_tags (std::vector<ustring> tags, ustring &out) {
    if (disabled) return false;

    for (PeasPluginInfo * p : thread_index_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (thread_index_extensions, p);

      char * tgs = astroid_threadindex_activatable_format_tags (ASTROID_THREADINDEX_ACTIVATABLE(pe), Glib::ListHandler<ustring>::vector_to_list (tags).data ());

      if (tgs != NULL) {
        out = ustring (tgs);
        return true;
      }
    }

    return false;
  }
}

