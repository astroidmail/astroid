# pragma once

# include <webkit/webkit.h>

namespace Astroid {

  extern "C" WebKitWebView * ThreadView_activate_inspector (
      WebKitWebInspector *,
      WebKitWebView *,
      gpointer );

  extern "C" bool ThreadView_show_inspector (
      WebKitWebInspector *,
      gpointer);

}

