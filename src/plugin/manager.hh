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
      class Extension {
        protected:
          PeasEngine        * engine;
          PeasExtensionSet  * extensions;
          bool active = false;

        public:
          Extension ();
          virtual ~Extension ();

          virtual void deactivate () = 0;
      };

      class ThreadIndexExtension : public Extension {
        private:
          ThreadIndex * thread_index;

        public:
          ThreadIndexExtension (ThreadIndex * ti);

          void deactivate () override;

          bool format_tags (std::vector<ustring> tags, ustring &out);
      };

      friend class ThreadIndexExtension;
      friend class Extension;

    protected:
      bool disabled, test;

  };
}

