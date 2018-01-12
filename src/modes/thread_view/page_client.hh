# pragma once

# include <webkit2/webkit2.h>
# include <glib.h>

namespace Astroid {

  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext *,
      gpointer);

  class PageClient {
    public:
      PageClient ();

      void init_web_extensions (WebKitWebContext * context);

    private:
      /* pipes */
      gint32 extension[2]; // extension writes
      gint32 client[2];    // client writes

  };

}

