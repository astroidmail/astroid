# pragma once

# include <webkit2/webkit2.h>

namespace Astroid {

  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext *,
      gpointer);

  class PageClient {
    public:
      PageClient ();

      void init_web_extensions (WebKitWebContext * context);

  };

}

