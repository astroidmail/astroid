# include <iostream>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "thread_view.hh"
# include "message_thread.hh"


using namespace std;

namespace Astroid {

  ThreadView::ThreadView () {
    tab_widget = new Gtk::Label (thread_id);

    pack_start (scroll, true, true, 5);

    /* set up webkit web view (using C api) */
    webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    gtk_container_add (GTK_CONTAINER (scroll.gobj()), GTK_WIDGET(webview));

    

    webkit_web_view_load_uri (webview, "http://gaute.vetsj.com");


    scroll.show_all ();
  }

  void ThreadView::load_thread (ustring _thread_id) {
    thread_id = _thread_id;

    ((Gtk::Label*) tab_widget)->set_text (thread_id);

    mthread = new MessageThread (thread_id);
    mthread->load_messages ();

  }

  void ThreadView::render () {

  }

  void ThreadView::grab_modal () {
    scroll.add_modal_grab ();
    scroll.grab_focus ();
  }

  void ThreadView::release_modal () {
    scroll.remove_modal_grab ();
  }

}

