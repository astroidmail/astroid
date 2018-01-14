# include "page_client.hh"
# include "utils/resource.hh"

# include <webkit2/webkit2.h>
# include <unistd.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <boost/filesystem.hpp>

using namespace boost::filesystem;

namespace Astroid {
  int PageClient::id = 0;

  PageClient::PageClient () {

    g_signal_connect (webkit_web_context_get_default (),
        "initialize-web-extensions",
        G_CALLBACK (PageClient_init_web_extensions),
        (gpointer) this);

  }

  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext * context,
      gpointer           user_data) {

    ((PageClient *) user_data)->init_web_extensions (context);
  }

  PageClient::~PageClient () {
    if (!socket_addr.empty () && exists (socket_addr.c_str ())) {
      unlink (socket_addr.c_str ());
    }
  }

  void PageClient::init_web_extensions (WebKitWebContext * context) {

    /* add path to Astroid web extension */
# ifdef DEBUG
    LOG (warn) << "pc: adding " << Resource::get_exe_dir ().c_str () << " to web extension search path.";

    webkit_web_context_set_web_extensions_directory (
        context,
        Resource::get_exe_dir().c_str());
# endif

    /* set up unix socket */
    id++;

    socket_addr = ustring::compose ("/tmp/astroid.%1", id);

    refptr<Gio::UnixSocketAddress> addr = Gio::UnixSocketAddress::create (socket_addr,
        Gio::UNIX_SOCKET_ADDRESS_PATH);

    refptr<Gio::SocketAddress> eaddr;

    LOG (debug) << "pc: socket: " << addr->get_path ();

    srv = Gio::SocketListener::create ();
    srv->add_address (addr, Gio::SocketType::SOCKET_TYPE_STREAM,
      Gio::SocketProtocol::SOCKET_PROTOCOL_DEFAULT,
      eaddr);

    /* listen */
    srv->accept_async (sigc::mem_fun (this, &PageClient::extension_connect));

    /* send socket address (TODO: include key) */
    GVariant * gaddr = g_variant_new_string (addr->get_path ().c_str ());

    webkit_web_context_set_web_extensions_initialization_user_data (
        context,
        gaddr);
  }

  void PageClient::extension_connect (refptr<Gio::AsyncResult> &res) {
    LOG (warn) << "pc: got extension connect";

    ext = refptr<Gio::UnixConnection>::cast_dynamic (srv->accept_finish (res));


    /* set up IOChannel */
    cli = Glib::IOChannel::create_from_fd (ext->get_socket()->get_fd ());
  }

  void PageClient::write () {
    cli->write ("asdf");
  }

}

