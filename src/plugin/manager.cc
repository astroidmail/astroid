# include <libpeas/peas.h>
# include <glibmm.h>
# include <vector>

# include "manager.hh"
# include "log.hh"
# include "build_config.hh"

# include "astroid_activatable.h"
# include "thread_index_activatable.h"

using std::endl;

/* remember to set GI_TYPELIB_PATH=$(pwd) when testing */

namespace Astroid {
  PluginManager::PluginManager (bool _disabled, bool _test) {
    log << info << "plugin: starting manager.." << endl;


    disabled = _disabled;
    test     = _test;

    if (disabled) {
      log << info << "plugin: disabled." << endl;
      return;
    }

    engine = peas_engine_get_default ();
    peas_engine_enable_loader (engine, "python3");


    if (test) {
      log << debug << "plugin: adding test path" << endl;
      peas_engine_prepend_search_path (engine, "/home/gaute/dev/mail/notm/astroid/test/test_home/plugins", NULL);
    }

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
    log << debug << "plugins: refreshing.." << endl;
    peas_engine_rescan_plugins (engine);

    const GList * ps = peas_engine_get_plugin_list (engine);

    log << debug << "plugins: found " << g_list_length ((GList *)ps) << " plugins." << endl;

    for (; ps != NULL; ps = ps->next) {
      auto p = (PeasPluginInfo *) ps->data;

      log << debug << "plugins: loading: " << peas_plugin_info_get_name (p) << endl;

      bool e = peas_engine_load_plugin (engine, p);

      if (e) {
        log << debug << "plugins: loaded: " << peas_plugin_info_get_name (p) << endl;

        PeasExtension * pe = peas_extension_set_get_extension (astroid_extensions, p);

        astroid_activatable_activate (ASTROID_ACTIVATABLE(pe));
        /* AstroidActivatable * ae = ASTROID_ACTIVATABLE ( pe ); */
        /* astroid_activatable_activate (ae); */

        /* char * k = astroid_activatable_ask (ae, "hello test"); */
        /* log << debug << "got: " << k << endl; */

      } else {
        log << error << "plugins: failed loading: " << peas_plugin_info_get_name (p) << endl;
      }

    }

    /*
    peas_extension_set_call (astroid_extensions, "activate");

    GValue a = G_VALUE_INIT;
    g_value_init (&a, G_TYPE_STRING);
    g_value_set_static_string (&a, "hello there..");


    peas_extension_set_call (astroid_extensions, "ask", &a);

    log << debug  << "got: " << g_value_get_string (&a) << endl;
    */

    /* std::vector<PeasPluginInfo> plugins = Glib::ListHandler<PeasPluginInfo>::list_to_vector (ps, Glib::OWNERSHIP_NONE); */


    /* GPluginPlugin * p = gplugin_manager_find_plugin ("astroid"); */

    /* GError *err = NULL; */
    /* bool e = gplugin_manager_load_plugin (p, &err); */


  }
}

