# include <gtkmm.h>
# include <webkit/webkit.h>

# include "message_thread.hh"
# include "db.hh"

# include "web_inspector.hh"
# include "thread_view.hh"

namespace Astroid {
  void ThreadViewInspector::setup (ThreadView * tv) {
    thread_view = tv;

    /* set up web view inspector */
    web_inspector = webkit_web_view_get_inspector (thread_view->webview);
    g_signal_connect (web_inspector, "inspect-web-view",
        G_CALLBACK(ThreadViewInspector_activate_inspector),
        (gpointer) this);

    g_signal_connect (web_inspector, "show-window",
        G_CALLBACK(ThreadViewInspector_show_inspector),
        (gpointer) this);
  }

  extern "C" WebKitWebView * ThreadViewInspector_activate_inspector (
      WebKitWebInspector * wi,
      WebKitWebView *          w,
      gpointer                 data )
  {
    return ((ThreadViewInspector *) data)->activate_inspector (wi, w);
  }


  WebKitWebView * ThreadViewInspector::activate_inspector (
      WebKitWebInspector     * /* web_inspector */,
      WebKitWebView          * /* web_view */)
  {
    /* set up inspector window */
    LOG (info) << "tv: starting conversation inspector..";
    inspector_window = new Gtk::Window ();
    inspector_window->set_default_size (600, 600);
    if (thread_view->thread)
      inspector_window->set_title (ustring::compose("Conversation inspector: %1", thread_view->thread->subject));
    inspector_window->add (inspector_scroll);
    WebKitWebView * inspector_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    gtk_container_add (GTK_CONTAINER (inspector_scroll.gobj()),
          GTK_WIDGET (inspector_view));

    return inspector_view;
  }

  extern "C" bool ThreadViewInspector_show_inspector (
      WebKitWebInspector * wi,
      gpointer data)
  {
    return ((ThreadViewInspector *) data)->show_inspector (wi);
  }

  bool ThreadViewInspector::show_inspector (WebKitWebInspector * /* wi */) {
    LOG (info) << "tv: show inspector..";

    inspector_window->show_all ();

    thread_view->release_modal ();

    return true;
  }
}

