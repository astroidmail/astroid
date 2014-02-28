# pragma once

# include <iostream>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  extern "C" bool ThreadView_on_load_changed (
          GtkWidget *,
          GParamSpec *,
          gpointer );

  class ThreadView : public Mode {
    /*
    friend bool ThreadView_on_load_changed (
          GObject *,
          WebKitLoadEvent,
          gchar *,
          gpointer,
          gpointer );
    */
    public:
      ThreadView ();
      ~ThreadView ();
      void load_thread (ustring);
      void render ();

      ustring thread_id;
      MessageThread * mthread;

      Gtk::ScrolledWindow scroll;

      /* webkit (using C api) */
      WebKitWebView     * webview;
      WebKitWebSettings * websettings;

      static bool theme_loaded;
      static const char *  thread_view_html_f;
      static const char *  thread_view_css_f;
      static ustring       thread_view_html;
      static ustring       thread_view_css;
      const char * STYLE_NAME = "STYLE";

      /* event wrappers */
      bool on_load_changed (
          GtkWidget *,
          GParamSpec *);

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

