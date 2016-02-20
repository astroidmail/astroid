# pragma once

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "proto.hh"

namespace Astroid {

  extern "C" WebKitWebView * ThreadViewInspector_activate_inspector (
      WebKitWebInspector *,
      WebKitWebView *,
      gpointer );

  extern "C" bool ThreadViewInspector_show_inspector (
      WebKitWebInspector *,
      gpointer);

  class ThreadViewInspector {
    public:
      void setup (ThreadView *);

      ThreadView * thread_view;

      WebKitWebInspector * web_inspector;

      Gtk::Window * inspector_window;
      Gtk::ScrolledWindow inspector_scroll;
      WebKitWebView * activate_inspector (
          WebKitWebInspector *,
          WebKitWebView *);
      bool show_inspector (WebKitWebInspector *);
  };

}

