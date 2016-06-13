# pragma once

# include <libpeas/peas.h>
# include <vector>

# include "astroid.hh"

# include "astroid_activatable.h"
# include "thread_index_activatable.h"

namespace Astroid {
  class PluginManager {
    public:
      PluginManager (bool disabled, bool test);
      ~PluginManager ();

      void refresh ();

      PeasEngine * engine;
      PeasExtensionSet * astroid_extensions;
      PeasExtensionSet * thread_index_extensions;

      std::vector<PeasPluginInfo *>  astroid_plugins;
      std::vector<PeasPluginInfo *>  thread_index_plugins;

      /* single end point plugins */
      bool get_avatar_uri (ustring email, ustring type, int size, ustring &out);

      /* thread index */
      bool thread_index_format_tags (std::vector<ustring> tags, ustring &out);


    private:
      bool disabled, test;

  };
}

