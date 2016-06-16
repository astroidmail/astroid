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

# include "modes/thread_index/thread_index.hh"

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
    peas_engine_enable_loader (engine, "lua5.1");

    bfs::path plugin_dir = astroid->standard_paths().plugin_dir;

    /* add installed location */
    if (!test) {
      bfs::path prefix_p (PREFIX);
      prefix_p = prefix_p / bfs::path ("share/astroid/plugins");
      log << debug << "plugins: adding path: " << prefix_p.c_str () << endl;
      peas_engine_prepend_search_path (engine, prefix_p.c_str (), NULL);

    } else {
      char * g = getenv ("GI_TYPELIB_PATH");
      if (g == NULL) {
        log << LogLevel::test << "plugins: setting GI_TYPELIB_PATH: " << bfs::current_path ().c_str () << endl;
        setenv ("GI_TYPELIB_PATH", bfs::current_path ().c_str (), 1);
      } else {
        log << error << "plugins: GI_TYPELIB_PATH already set, not touching.." << endl;
      }

    }

    /* adding local plugins */
    log << debug << "plugin: adding path: " << plugin_dir.c_str () << endl;
    peas_engine_prepend_search_path (engine, plugin_dir.c_str (), NULL);

    astroid_extensions = peas_extension_set_new (engine, ASTROID_TYPE_ACTIVATABLE, NULL);

    refresh ();
  }

  PluginManager::~PluginManager () {
    if (!disabled) {
      log << debug << "plugins: uninit." << endl;
      for (PeasPluginInfo * p : astroid_plugins) {
        PeasExtension * pe = peas_extension_set_get_extension (astroid_extensions, p);
        astroid_activatable_deactivate (ASTROID_ACTIVATABLE(pe));

      }
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

        if (peas_engine_provides_extension (engine, p, ASTROID_TYPE_ACTIVATABLE)) {
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

        } else if (peas_engine_provides_extension (engine, p, ASTROID_THREADINDEX_TYPE_ACTIVATABLE)) {
          log << debug << "plugins: registering threadindex plugin.." << endl;

          thread_index_plugins.push_back (p);

        } else {
          log << error << "plugin: " << peas_plugin_info_get_name (p) << " does not implement any known extension." << endl;
        }


      } else {
        log << error << "plugins: failed loading: " << peas_plugin_info_get_name (p) << endl;
      }
    }
  }

  /* ********************
   * Extension
   * ********************/
  PluginManager::Extension::Extension () {
    engine        = astroid->plugin_manager->engine;

  }

  PluginManager::Extension::~Extension () {
    /* make sure all extensions have been deactivated in subclass destructor */
    log << debug << "extension: destruct." << endl;
    g_object_unref (extensions);
  }

  /* ********************
   * ThreadIndexExtension
   * ********************/

  PluginManager::ThreadIndexExtension::ThreadIndexExtension (ThreadIndex * ti) {
    thread_index  = ti;

    /* loading extensions for each plugin */
    extensions = peas_extension_set_new (engine, ASTROID_THREADINDEX_TYPE_ACTIVATABLE, "thread_index", ti->gobj (), NULL);

    for ( PeasPluginInfo *p : astroid->plugin_manager->thread_index_plugins) {

      log << debug << "plugins: activating threadindex plugin: " << peas_plugin_info_get_name (p) << endl;

      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_THREADINDEX_ACTIVATABLE( pe)) {
        astroid_threadindex_activatable_activate (ASTROID_THREADINDEX_ACTIVATABLE(pe));
      }
    }

    active = true;
  }

  void PluginManager::ThreadIndexExtension::deactivate () {
    active = false;

    for ( PeasPluginInfo *p : astroid->plugin_manager->thread_index_plugins) {

      log << debug << "plugins: deactivating: " << peas_plugin_info_get_name (p) << endl;
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_THREADINDEX_ACTIVATABLE( pe)) {
        astroid_threadindex_activatable_deactivate (ASTROID_THREADINDEX_ACTIVATABLE(pe));
      }
    }
  }

  bool PluginManager::ThreadIndexExtension::format_tags (std::vector<ustring> tags, ustring &out) {
    if (!active || astroid->plugin_manager->disabled) return false;

    for (PeasPluginInfo * p : astroid->plugin_manager->thread_index_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (pe) {

        char * tgs = astroid_threadindex_activatable_format_tags (ASTROID_THREADINDEX_ACTIVATABLE(pe), Glib::ListHandler<ustring>::vector_to_list (tags).data ());

        if (tgs != NULL) {
          out = ustring (tgs);
          return true;
        }
      }
    }

    return false;
  }

  /* ************************
   * General plugin functions
   * ************************/

  bool PluginManager::get_avatar_uri (ustring email, ustring type, int size, ustring &out) {
    if (disabled) return false;

    for (PeasPluginInfo * p : astroid_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (astroid_extensions, p);

      char * uri = astroid_activatable_get_avatar_uri (ASTROID_ACTIVATABLE(pe), email.c_str (), type.c_str (), size);

      if (uri != NULL) {
        out = ustring (uri);
        return true;
      }
    }

    return false;
  }

  std::vector<ustring> PluginManager::get_allowed_uris () {
    std::vector<ustring> uris;

    if (disabled) return uris;

    for (PeasPluginInfo * p : astroid_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (astroid_extensions, p);

      GList * muris = astroid_activatable_get_allowed_uris (ASTROID_ACTIVATABLE(pe));

      if (muris != NULL) {
        std::vector<ustring> _muris = Glib::ListHandler<ustring>::list_to_vector (muris, Glib::OWNERSHIP_NONE);
        uris.insert (uris.end (), _muris.begin (), _muris.end ());
      }
    }

    return uris;
  }
}

