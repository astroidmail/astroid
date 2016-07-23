# pragma once

# include <libpeas/peas.h>
# include <vector>

# include "astroid.hh"
# include "proto.hh"

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
      std::vector<PeasPluginInfo *>  thread_view_plugins;

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

          bool format_tags (std::vector<ustring> tags, ustring bg, bool selected, ustring &out);
      };

      class ThreadViewExtension : public Extension {
        private:
          ThreadView * thread_view;

        public:
          ThreadViewExtension (ThreadView * ti);

          void deactivate () override;

          std::vector<ustring> get_allowed_uris ();
          bool get_avatar_uri (ustring email, ustring type, int size, refptr<Message> m, ustring &out);
          bool format_tags (std::vector<ustring> tags, ustring bg, bool selected, ustring &out);

      };

      friend class ThreadIndexExtension;
      friend class ThreadViewExtension;
      friend class Extension;

    protected:
      bool disabled, test;

  };
}

