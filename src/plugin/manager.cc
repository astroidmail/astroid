# include <libpeas/peas.h>
# include <glibmm.h>
# include <vector>
# include <cstdlib>

# include <boost/filesystem.hpp>

# include "manager.hh"
# include "astroid.hh"
# include "config.hh"
# include "build_config.hh"
# include "utils/vector_utils.hh"
# include "message_thread.hh"

# include "astroid_activatable.h"
# include "thread_index_activatable.h"
# include "thread_view_activatable.h"

# include "modes/thread_index/thread_index.hh"
# include "modes/thread_view/thread_view.hh"

using std::endl;
namespace bfs = boost::filesystem;

/* remember to set GI_TYPELIB_PATH=$(pwd) when testing */

namespace Astroid {
  PluginManager::PluginManager (bool _disabled, bool _test) {
    LOG (info) << "plugins: starting manager..";


    disabled = _disabled;
    test     = _test;

    if (disabled) {
      LOG (info) << "plugins: disabled.";
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
      LOG (debug) << "plugins: adding path: " << prefix_p.c_str ();
      peas_engine_prepend_search_path (engine, prefix_p.c_str (), NULL);

    } else {
      char * g = getenv ("GI_TYPELIB_PATH");
      if (g == NULL) {
        LOG (warn) << "plugins: setting GI_TYPELIB_PATH: " << bfs::current_path ().c_str ();
        setenv ("GI_TYPELIB_PATH", bfs::current_path ().c_str (), 1);
      } else {
        LOG (error) << "plugins: GI_TYPELIB_PATH already set, not touching..";
      }
    }

    /* set env ASTROID_CONFIG for plugins */
    setenv ("ASTROID_CONFIG", astroid->standard_paths().config_file.c_str (), 1);

    /* adding local plugins */
    LOG (debug) << "plugin: adding path: " << plugin_dir.c_str ();
    peas_engine_prepend_search_path (engine, plugin_dir.c_str (), NULL);

    refresh ();
  }

  PluginManager::~PluginManager () {
  }

  void PluginManager::refresh () {
    if (disabled) return;

    LOG (debug) << "plugins: refreshing..";
    peas_engine_rescan_plugins (engine);

    const GList * ps = peas_engine_get_plugin_list (engine);

    LOG (debug) << "plugins: found " << g_list_length ((GList *) ps) << " plugins.";

    for (; ps != NULL; ps = ps->next) {
      auto p = (PeasPluginInfo *) ps->data;

      LOG (debug) << "plugins: loading: " << peas_plugin_info_get_name (p);

      bool e = peas_engine_load_plugin (engine, p);

      if (e) {
        LOG (debug) << "plugins: loaded: " << peas_plugin_info_get_name (p);

        bool found = false;

        /* a plugin might implement more than one extension */

        if (peas_engine_provides_extension (engine, p, ASTROID_TYPE_ACTIVATABLE)) {
          LOG (debug) << "plugins: registering astroid plugin..";

          astroid_plugins.push_back (p);
          found = true;
        }

        if (peas_engine_provides_extension (engine, p, ASTROID_THREADINDEX_TYPE_ACTIVATABLE)) {
          LOG (debug) << "plugins: registering threadindex plugin..";

          thread_index_plugins.push_back (p);
          found = true;
        }

        if (peas_engine_provides_extension (engine, p, ASTROID_THREADVIEW_TYPE_ACTIVATABLE)) {
          LOG (debug) << "plugins: registering threadview plugin..";

          thread_view_plugins.push_back (p);
          found = true;
        }

        if (!found) {
          LOG (error) << "plugin: " << peas_plugin_info_get_name (p) << " does not implement any known extension.";
        }


      } else {
        LOG (error) << "plugins: failed loading: " << peas_plugin_info_get_name (p);
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
    LOG (debug) << "extension: destruct.";
    if (extensions) g_object_unref (extensions);
  }

  /* ********************
   * AstroidExtension
   * ********************/
  PluginManager::AstroidExtension::AstroidExtension (Astroid * a) {
    astroid = a;

    if (astroid->plugin_manager->disabled) return;

    /* loading extensions for each plugin */
    extensions = peas_extension_set_new (engine, ASTROID_TYPE_ACTIVATABLE, NULL);

    for ( PeasPluginInfo *p : astroid->plugin_manager->astroid_plugins) {

      LOG (debug) << "plugins: activating astroid plugin: " << peas_plugin_info_get_name (p);

      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_ACTIVATABLE( pe)) {
        astroid_activatable_activate (ASTROID_ACTIVATABLE(pe));
      }
    }

    active = true;
  }

  void PluginManager::AstroidExtension::deactivate () {
    active = false;

    for ( PeasPluginInfo *p : astroid->plugin_manager->astroid_plugins) {

      LOG (debug) << "plugins: deactivating: " << peas_plugin_info_get_name (p);
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_ACTIVATABLE( pe)) {
        astroid_activatable_deactivate (ASTROID_ACTIVATABLE(pe));
      }
    }
  }

  bool PluginManager::AstroidExtension::get_user_agent (ustring &out) {
    if (!active || astroid->plugin_manager->disabled) return false;

    for (PeasPluginInfo * p : astroid->plugin_manager->astroid_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      const char * ua = astroid_activatable_get_user_agent (ASTROID_ACTIVATABLE(pe));

      if (ua != NULL) {
        out = ustring (ua);
        return true;
      }
    }

    return false;
  }

  bool PluginManager::AstroidExtension::generate_mid (ustring &out) {
    if (!active || astroid->plugin_manager->disabled) return false;

    for (PeasPluginInfo * p : astroid->plugin_manager->astroid_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      const char * ua = astroid_activatable_generate_mid (ASTROID_ACTIVATABLE(pe));

      if (ua != NULL) {
        out = ustring (ua);
        return true;
      }
    }

    return false;
  }

  /* ********************
   * ThreadIndexExtension
   * ********************/

  PluginManager::ThreadIndexExtension::ThreadIndexExtension (ThreadIndex * ti) {
    thread_index  = ti;

    if (astroid->plugin_manager->disabled) return;

    /* loading extensions for each plugin */
    extensions = peas_extension_set_new (engine, ASTROID_THREADINDEX_TYPE_ACTIVATABLE, "thread_index", ti->gobj (), NULL);

    for ( PeasPluginInfo *p : astroid->plugin_manager->thread_index_plugins) {

      LOG (debug) << "plugins: activating threadindex plugin: " << peas_plugin_info_get_name (p);

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

      LOG (debug) << "plugins: deactivating: " << peas_plugin_info_get_name (p);
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_THREADINDEX_ACTIVATABLE( pe)) {
        astroid_threadindex_activatable_deactivate (ASTROID_THREADINDEX_ACTIVATABLE(pe));
      }
    }
  }

  bool PluginManager::ThreadIndexExtension::format_tags (
      std::vector<ustring> tags,
      ustring bg,
      bool selected,
      ustring &out) {
    if (!active || astroid->plugin_manager->disabled) return false;

    for (PeasPluginInfo * p : astroid->plugin_manager->thread_index_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (pe) {

        char * tgs = astroid_threadindex_activatable_format_tags (ASTROID_THREADINDEX_ACTIVATABLE(pe), bg.c_str (), Glib::ListHandler<ustring>::vector_to_list (tags).data (), selected);

        if (tgs != NULL) {
          out = ustring (tgs);
          return true;
        }
      }
    }

    return false;
  }

  /* ************************
   * ThreadViewExtension
   * ************************/
  PluginManager::ThreadViewExtension::ThreadViewExtension (ThreadView * tv) {
    thread_view  = tv;

    if (astroid->plugin_manager->disabled) return;

    /* loading extensions for each plugin */
    extensions = peas_extension_set_new (engine, ASTROID_THREADVIEW_TYPE_ACTIVATABLE, "thread_view", tv->gobj (), "web_view", tv->webview, NULL);

    for ( PeasPluginInfo *p : astroid->plugin_manager->thread_view_plugins) {

      LOG (debug) << "plugins: activating threadview plugin: " << peas_plugin_info_get_name (p);

      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_THREADVIEW_ACTIVATABLE( pe)) {
        astroid_threadview_activatable_activate (ASTROID_THREADVIEW_ACTIVATABLE(pe));
      }
    }

    active = true;
  }

  void PluginManager::ThreadViewExtension::deactivate () {
    active = false;

    for ( PeasPluginInfo *p : astroid->plugin_manager->thread_view_plugins) {

      LOG (debug) << "plugins: deactivating: " << peas_plugin_info_get_name (p);
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (ASTROID_IS_THREADVIEW_ACTIVATABLE( pe)) {
        astroid_threadview_activatable_deactivate (ASTROID_THREADVIEW_ACTIVATABLE(pe));
      }
    }
  }

  bool PluginManager::ThreadViewExtension::get_avatar_uri (ustring email, ustring type, int size, refptr<Message> m, ustring &out) {
    if (!active || astroid->plugin_manager->disabled) return false;

    for (PeasPluginInfo * p : astroid->plugin_manager->thread_view_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      char * uri = astroid_threadview_activatable_get_avatar_uri (ASTROID_THREADVIEW_ACTIVATABLE(pe), email.c_str (), type.c_str (), size, m->message);

      if (uri != NULL) {
        out = ustring (uri);
        return true;
      }
    }

    return false;
  }

  std::vector<ustring> PluginManager::ThreadViewExtension::get_allowed_uris () {
    std::vector<ustring> uris;

    if (!active || astroid->plugin_manager->disabled) return uris;

    for (PeasPluginInfo * p : astroid->plugin_manager->thread_view_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      GList * muris = astroid_threadview_activatable_get_allowed_uris (ASTROID_THREADVIEW_ACTIVATABLE(pe));

      if (muris != NULL) {
        std::vector<ustring> _muris = Glib::ListHandler<ustring>::list_to_vector (muris, Glib::OWNERSHIP_NONE);
        uris.insert (uris.end (), _muris.begin (), _muris.end ());
      }
    }

    return uris;
  }

  bool PluginManager::ThreadViewExtension::format_tags (
      std::vector<ustring> tags,
      ustring bg,
      bool selected,
      ustring &out) {
    if (!active || astroid->plugin_manager->disabled) return false;

    for (PeasPluginInfo * p : astroid->plugin_manager->thread_view_plugins) {
      PeasExtension * pe = peas_extension_set_get_extension (extensions, p);

      if (pe) {

        char * tgs = astroid_threadview_activatable_format_tags (ASTROID_THREADVIEW_ACTIVATABLE(pe), bg.c_str (), Glib::ListHandler<ustring>::vector_to_list (tags).data (), selected);

        if (tgs != NULL) {
          out = ustring (tgs);
          return true;
        }
      }
    }

    return false;
  }
}

