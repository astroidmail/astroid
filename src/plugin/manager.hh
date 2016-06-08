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

    private:
      bool disabled, test;

      PeasEngine * engine;
      PeasExtensionSet * astroid_extensions;

  };
}

