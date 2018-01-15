# pragma once

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <thread>

# include "messages.pb.h"

# define refptr Glib::RefPtr

extern "C" {

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data);

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (WebKitWebExtension *extension, gpointer pipes);


}

class AstroidExtension {
  public:
    AstroidExtension (WebKitWebExtension *, gpointer);

    void page_created (WebKitWebExtension *, WebKitWebPage *, gpointer);

  private:
    WebKitWebExtension * extension;
    WebKitWebPage * page;

    refptr<Gio::SocketClient> cli;
    refptr<Gio::SocketConnection> sock;

    refptr<Gio::InputStream>  istream;
    refptr<Gio::OutputStream> ostream;

    std::thread reader_t;
    void        reader ();
    bool        run = true;


    void handle_mark (AstroidMessages::Mark &m);
};

AstroidExtension * ext;

