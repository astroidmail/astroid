# include "page_client.hh"
# include "utils/resource.hh"

# include <webkit2/webkit2.h>

namespace Astroid {

  PageClient::PageClient () {

    g_signal_connect (webkit_web_context_get_default (),
        "initialize-web-extensions",
        G_CALLBACK (PageClient_init_web_extensions),
        (gpointer) this);

  }

  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext * context,
      gpointer           user_data) {

    ((PageClient *) user_data)->init_web_extensions (context);
  }


  void PageClient::init_web_extensions (WebKitWebContext * context) {

    /* add path to Astroid web extension */
# ifdef DEBUG
    LOG (warn) << "pc: adding " << Resource::get_exe_dir ().c_str () << " to web extension search path.";

    webkit_web_context_set_web_extensions_directory (
        context,
        Resource::get_exe_dir().c_str());
# endif
  }

}

