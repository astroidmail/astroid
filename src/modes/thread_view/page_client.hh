# pragma once

# include <webkit2/webkit2.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <thread>

# include "astroid.hh"

namespace AstroidMessages {
  class Message; // prototype
}

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

      /* ThreadView interface */
      void add_message (refptr<Message> m);

      void set_marked_state (refptr<Message> m, bool marked);
      void set_hidden_state (refptr<Message> m, bool hidden);
      void focus (refptr<Message> m);

    private:
      AstroidMessages::Message make_message (refptr<Message> m);

    private:
      static int id;

      ustring socket_addr;

      refptr<Gio::SocketListener> srv;
      refptr<Gio::UnixConnection> ext;
      void extension_connect (refptr<Gio::AsyncResult> &res);

      refptr<Gio::InputStream>  istream;
      refptr<Gio::OutputStream> ostream;

      void        reader ();
      std::thread reader_t;
      bool        run;
  };

}

