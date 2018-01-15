# include "page_client.hh"
# include "utils/resource.hh"

# include <webkit2/webkit2.h>
# include <unistd.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <boost/filesystem.hpp>
# include <iostream>
# include <thread>

# include "astroid.hh"
# include "modes/thread_view/webextension/ae_protocol.hh"
# include "messages.pb.h"

# include "message_thread.hh"

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

    run = false;
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

    istream = ext->get_input_stream ();
    ostream = ext->get_output_stream ();

    /* setting up reader */
    reader_t = std::thread (&PageClient::reader, this);
  }

  void PageClient::reader () {
    while (run) {
      gchar buffer[2049]; buffer[0] = '\0';
      gsize read = 0;

      /* read size of message */
      gsize sz;
      read = istream->read ((char*)&sz, sizeof (sz));

      if (read != sizeof(sz)) break;;

      /* read message type */
      AeProtocol::MessageTypes mt;
      read = istream->read ((char*)&mt, sizeof (mt));
      if (read != sizeof (mt)) break;

      /* read message */
      bool s = istream->read_all (buffer, sz, read);

      if (!s) break;

      /* parse message */
      switch (mt) {
        case AeProtocol::MessageTypes::Debug:
          {
            AstroidMessages::Debug m;
            m.ParseFromString (buffer);
            LOG (debug) << "pc: ae: " << m.msg ();
          }
          break;

        default:
          break; // unknown message
      }
    }
  }

  void PageClient::write () {
    AstroidMessages::Debug m;
    m.set_msg ("here is the message!");

    AeProtocol::send_message (AeProtocol::MessageTypes::Debug, m, ostream);
  }

  void PageClient::update_marked_state (refptr<Message> m, bool marked) {
    AstroidMessages::Mark msg;
    msg.set_mid (m->safe_mid ());
    msg.set_marked (marked);

    AeProtocol::send_message (AeProtocol::MessageTypes::Mark, msg, ostream);
  }

}

