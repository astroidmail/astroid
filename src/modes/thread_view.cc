# include <iostream>

# include <gtkmm.h>
# include <webkit2/webkit2.h>

# include "thread_view.hh"
# include "message_thread.hh"


using namespace std;

namespace Astroid {

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
  }

  ThreadView::~ThreadView () {
    g_object_unref (webview);
    g_object_unref (websettings);
  }

  void ThreadView::load_thread (ustring _thread_id) {
    thread_id = _thread_id;

    ((Gtk::Label*) tab_widget)->set_text (thread_id);

    mthread = new MessageThread (thread_id);
    mthread->load_messages ();
  }

  void ThreadView::render () {
    webkit_web_view_load_html (webview, mthread->messages[0]->body().c_str(), "/tmp/");

  }

  void ThreadView::grab_modal () {
    //gtk_grab_add (GTK_WIDGET (webview));
    //gtk_widget_grab_focus (GTK_WIDGET (webview));
  }

  void ThreadView::release_modal () {
    //gtk_grab_remove (GTK_WIDGET (webview));
  }

}

