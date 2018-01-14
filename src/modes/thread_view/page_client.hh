# pragma once

# include <webkit2/webkit2.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>

# include "astroid.hh"

namespace Astroid {

  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext *,
      gpointer);

  class PageClient : public sigc::trackable {
    public:
      PageClient ();
      ~PageClient ();

      void init_web_extensions (WebKitWebContext * context);
      void write ();

    private:
      static int id;

      ustring socket_addr;

      refptr<Glib::IOChannel> cli;

      refptr<Gio::SocketListener> srv;
      refptr<Gio::UnixConnection> ext;
      void extension_connect (refptr<Gio::AsyncResult> &res);

      /* bool cli_read (Glib::IOCondition); */
  };

}

