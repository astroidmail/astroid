# include "page_client.hh"
# include "utils/resource.hh"

# include <webkit2/webkit2.h>
# include <unistd.h>
# include <glib.h>

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

    /* set up pipes */
    pipe (extension);
    pipe (client);

    // TODO: Close pipes

    GVariant * ext_read  = g_variant_new ("%d", (gint32) client[0]);
    GVariant * ext_write = g_variant_new ("%d", (gint32) extension[1]);
    GVariant * ext_pipes_a[2] = { ext_read, ext_write};

    GVariant * ext_pipes = g_variant_new_tuple (ext_pipes_a, 2);

    webkit_web_context_set_web_extensions_initialization_user_data (
        context,
        ext_pipes);
  }

}

