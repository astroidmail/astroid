# include "log.hh"
# include "message_thread.hh"
# include "db.hh"

# include "web_inspector.hh"
# include "thread_view.hh"

namespace Astroid {
  extern "C" WebKitWebView * ThreadView_activate_inspector (
      WebKitWebInspector * wi,
      WebKitWebView *          w,
      gpointer                 data )
  {
    return ((ThreadView *) data)->activate_inspector (wi, w);
  }


  WebKitWebView * ThreadView::activate_inspector (
      WebKitWebInspector     * web_inspector,
      WebKitWebView          * web_view)
  {
    /* set up inspector window */
    log << info << "tv: starting conversation inspector.." << endl;
    inspector_window = new Gtk::Window ();
    inspector_window->set_default_size (600, 600);
    if (thread)
      inspector_window->set_title (ustring::compose("Conversation inspector: %1", thread->subject));
    inspector_window->add (inspector_scroll);
    WebKitWebView * inspector_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    gtk_container_add (GTK_CONTAINER (inspector_scroll.gobj()),
          GTK_WIDGET (inspector_view));

    return inspector_view;
  }

  extern "C" bool ThreadView_show_inspector (
      WebKitWebInspector * wi,
      gpointer data)
  {
    return ((ThreadView *) data)->show_inspector (wi);
  }

  bool ThreadView::show_inspector (WebKitWebInspector * wi) {
    log << info << "tv: show inspector.." << endl;

    inspector_window->show_all ();

    release_modal ();

    return true;
  }
}

