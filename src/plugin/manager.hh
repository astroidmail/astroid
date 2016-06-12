# pragma once

# include <libpeas/peas.h>
# include <vector>

# include "astroid.hh"

namespace Astroid {
  class PluginManager {
    public:
      PluginManager (bool disabled, bool test);
      ~PluginManager ();

      void refresh ();

      PeasEngine * engine;
      PeasExtensionSet * astroid_extensions;
      PeasExtensionSet * thread_index_extensions;

    private:
      bool disabled, test;

  };
}

