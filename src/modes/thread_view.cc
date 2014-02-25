# include <iostream>
# include <fstream>

# include <gtkmm.h>
# include <webkit2/webkit2.h>

# include "thread_view.hh"
# include "message_thread.hh"


using namespace std;

namespace Astroid {
  bool ThreadView::theme_loaded = false;
  const char * ThreadView::thread_view_html_f = "ui/thread-view.html";
  const char * ThreadView::thread_view_css_f  = "ui/thread-view.css";
  ustring ThreadView::thread_view_html;
  ustring ThreadView::thread_view_css;


  ThreadView::ThreadView () {
    tab_widget = new Gtk::Label (thread_id);

    pack_start (scroll, true, true, 5);

    /* set up webkit web view (using C api) */
    webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    websettings = WEBKIT_SETTINGS (webkit_settings_new ());
    webkit_settings_set_auto_load_images (websettings, false);
    webkit_settings_set_enable_html5_database (websettings, false);
    webkit_settings_set_enable_html5_local_storage (websettings, false);
    webkit_settings_set_enable_hyperlink_auditing (websettings, false);
    webkit_settings_set_enable_java (websettings, false);
    webkit_settings_set_enable_javascript (websettings, false);
    webkit_settings_set_enable_plugins (websettings, false);
    webkit_settings_set_enable_xss_auditor (websettings, false);


    //webkit_settings_set_enable_spatial_navigation (websettings, true);
    webkit_settings_set_media_playback_requires_user_gesture (websettings, true);
    webkit_settings_set_enable_webaudio (websettings, true);
    webkit_settings_set_enable_webgl (websettings, true);
    webkit_settings_set_enable_private_browsing (websettings, true);
    webkit_settings_set_enable_fullscreen (websettings, true);

    webkit_web_view_set_settings (webview, websettings);


    gtk_container_add (GTK_CONTAINER (scroll.gobj()), GTK_WIDGET(webview));


    scroll.show_all ();

    /* load css, html and DOM objects */
    if (!theme_loaded) {
      ifstream tv_html_f (thread_view_html_f);
      istreambuf_iterator<char> eos; // default is eos
      istreambuf_iterator<char> tv_iit (tv_html_f);

      thread_view_html.append (tv_iit, eos);
      tv_html_f.close ();

      ifstream tv_css_f (thread_view_css_f);
      istreambuf_iterator<char> tv_css_iit (tv_css_f);
      thread_view_css.append (tv_css_iit, eos);
      tv_css_f.close ();

      theme_loaded = true;
    }

    g_signal_connect (webview, "load-changed",
        G_CALLBACK(ThreadView::on_load_changed), (gpointer) this);
    webkit_web_view_load_html (webview, thread_view_html.c_str (), "/tmp/");

  }

  ThreadView::~ThreadView () {
    g_object_unref (webview);
    g_object_unref (websettings);
  }

  bool ThreadView::on_load_changed (
      GObject * o,
      WebKitLoadEvent ev,
      gchar * failing_url,
      gpointer error,
      gpointer data )
  {
    return ((ThreadView *) data)->on_load_changed_int (o, ev, failing_url,
        error);
  }

  bool ThreadView::on_load_changed_int (
      GObject *o,
      WebKitLoadEvent ev,
      gchar * failing_url,
      gpointer error )
  {
    cout << "tv: on_load_changed: " << ev << endl;
    switch (ev) {
      case WEBKIT_LOAD_FINISHED:
        cout << "tv: load finished." << endl;
        break;
    }

    return true;
  }

  void ThreadView::load_thread (ustring _thread_id) {
    thread_id = _thread_id;

    ((Gtk::Label*) tab_widget)->set_text (thread_id);

    mthread = new MessageThread (thread_id);
    mthread->load_messages ();
  }

  void ThreadView::render () {
    //webkit_web_view_load_html (webview, mthread->messages[0]->body().c_str(), "/tmp/");




  }

  void ThreadView::grab_modal () {
    //gtk_grab_add (GTK_WIDGET (webview));
    //gtk_widget_grab_focus (GTK_WIDGET (webview));
  }

  void ThreadView::release_modal () {
    //gtk_grab_remove (GTK_WIDGET (webview));
  }

}

