# pragma once

# include <libpeas/peas.h>
# include <vector>

# include "astroid.hh"
# include "proto.hh"

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

      std::vector<PeasPluginInfo *>  astroid_plugins;
      std::vector<PeasPluginInfo *>  thread_index_plugins;

      /* single end-point plugins */
      std::vector<ustring> get_allowed_uris ();
      bool get_avatar_uri (ustring email, ustring type, int size, ustring &out);

      /* thread index */
      class ThreadIndexExtension {
        private:
          ThreadIndex * thread_index;
          PeasEngine  * engine;
          PeasExtensionSet * thread_index_extensions;

        public:
          ThreadIndexExtension (ThreadIndex * ti);
          ~ThreadIndexExtension ();

          bool format_tags (std::vector<ustring> tags, ustring &out);
      };

      friend class ThreadIndexExtension;

    protected:
      bool disabled, test;

  };
}

