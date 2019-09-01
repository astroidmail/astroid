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

      std::vector<PeasPluginInfo *>  astroid_plugins;
      std::vector<PeasPluginInfo *>  thread_index_plugins;
      std::vector<PeasPluginInfo *>  thread_view_plugins;

      class Extension {
        protected:
          PeasEngine        * engine = NULL;
          PeasExtensionSet  * extensions = NULL;
          bool active = false;

        public:
          Extension ();
          virtual ~Extension ();

          virtual void deactivate () = 0;
      };

      class AstroidExtension : public Extension {
        private:
          Astroid * astroid;

        public:
          AstroidExtension (Astroid * a);

          void deactivate () override;

          bool get_user_agent (ustring &);
          bool generate_mid (ustring &);
          std::pair<ustring, ustring> get_tag_colors (ustring tag, ustring bg);
          std::vector<std::pair<ustring, ustring>> get_queries ();
          GMimeStream * process (const char * fname);
      };

      AstroidExtension * astroid_extension; // set up from Astroid

      class ThreadIndexExtension : public Extension {
        private:
          ThreadIndex * thread_index;

        public:
          ThreadIndexExtension (ThreadIndex * ti);

          void deactivate () override;

          bool format_tags (std::vector<ustring> tags, ustring bg, bool selected, ustring &out);
      };

      class ThreadViewExtension : public Extension {

        public:
          ThreadViewExtension (ThreadView * ti);

          void deactivate () override;

          std::vector<ustring> get_allowed_uris ();
          bool get_avatar_uri (ustring email, ustring type, int size, refptr<Message> m, ustring &out);
          bool format_tags (std::vector<ustring> tags, ustring bg, bool selected, ustring &out);
          std::string filter_part (std::string input_text, std::string input_html, std::string mime_type, bool is_patch);

      };

      friend class ThreadIndexExtension;
      friend class ThreadViewExtension;
      friend class AstroidExtension;
      friend class Extension;

    protected:
      bool disabled, test;

  };
}

