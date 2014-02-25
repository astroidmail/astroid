# pragma once

# include <iostream>

# include <gtkmm.h>
# include <webkit2/webkit2.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  class ThreadView : public Mode {
    public:
      ThreadView ();
      ~ThreadView ();
      void load_thread (ustring);
      void render ();

      ustring thread_id;
      MessageThread * mthread;

      Gtk::ScrolledWindow scroll;

      /* webkit (using C api) */
      WebKitWebView   * webview;
      WebKitSettings  * websettings;

      static bool theme_loaded;
      static const char *  thread_view_html_f;
      static const char *  thread_view_css_f;
      static ustring       thread_view_html;
      static ustring       thread_view_css;

      /* event wrappers (ugly) */
      static bool on_load_changed (
          GObject *,
          WebKitLoadEvent,
          gchar *,
          gpointer,
          gpointer );

      bool on_load_changed_int (
          GObject *,
          WebKitLoadEvent,
          gchar *,
          gpointer );

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
  };
}

