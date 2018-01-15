# include "tvextension.hh"

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>
# include <iostream>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>

# include "messages.pb.h"

extern "C" {

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data )
{
  ext->page_created (extension, web_page, user_data);
}

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (
    WebKitWebExtension *extension,
    gpointer pipes)
{
  ext = new AstroidExtension (extension, pipes);

  g_signal_connect (extension, "page-created",
      G_CALLBACK (web_page_created_callback),
      NULL);

}
}

AstroidExtension::AstroidExtension (WebKitWebExtension * e,
    gpointer gaddr) {
  extension = e;

  using std::cout;
  using std::endl;

  std::cout << "ae: inititalize" << std::endl;

  Gio::init ();

  /* retrieve socket address */
  std::cout << "type: " << g_variant_get_type_string ((GVariant *)gaddr) << std::endl;

  gsize sz;
  const char * caddr = g_variant_get_string ((GVariant *) gaddr, &sz);
  std::cout << "addr: " << caddr << std::endl;

  refptr<Gio::UnixSocketAddress> addr = Gio::UnixSocketAddress::create (caddr,
      Gio::UNIX_SOCKET_ADDRESS_PATH);

  /* connect to socket */
  std::cout << "ae: connecting.." << std::endl;
  cli = Gio::SocketClient::create ();

  try {
    sock = cli->connect (addr);
  } catch (Gio::Error &ex) {
    cout << "ae: error: " << ex.what () << endl;
  }


  ext_io = Glib::IOChannel::create_from_fd (sock->get_socket()->get_fd ());

  Glib::signal_io().connect (sigc::mem_fun (this, &AstroidExtension::ext_read_event), ext_io, Glib::IO_IN | Glib::IO_HUP);

  std::cout << "ae: init done" << std::endl;
}

bool AstroidExtension::ext_read_event (Glib::IOCondition cond) {
  if (cond == Glib::IO_HUP) {
    ext_io.clear();
    /* LOG (debug) << "poll: (stdout) got HUP"; */
    return false;
  }

  if ((cond & Glib::IO_IN) == 0) {
    /* LOG (error) << "poll: invalid fifo response"; */
    ext_io.clear ();
    return false;
  } else {
    Glib::ustring buf;

    ext_io->read_line(buf);
    if (*(--buf.end()) == '\n') buf.erase (--buf.end());

    std::cout << "ae: " << buf << std::endl;

  }
  return true;
}

void AstroidExtension::page_created (WebKitWebExtension * /* extension */,
    WebKitWebPage * _page,
    gpointer /* user_data */) {

  page = _page;

  std::cout << "ae: page created" << std::endl;

  g_print ("Page %d created for %s\n",
           (int) webkit_web_page_get_id (page),
           webkit_web_page_get_uri (page));
}

