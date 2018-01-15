# include "tvextension.hh"

# include <webkit2/webkit-web-extension.h>

# include <gmodule.h>
# include <iostream>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <thread>

# include "modes/thread_view/webextension/ae_protocol.hh"
# include "modes/thread_view/webextension/dom_utils.hh"
# include "messages.pb.h"


using std::cout;
using std::endl;

using namespace Astroid;

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

    istream = sock->get_input_stream ();
    ostream = sock->get_output_stream ();

    /* setting up reader thread */
    reader_t = std::thread (&AstroidExtension::reader, this);

  } catch (Gio::Error &ex) {
    cout << "ae: error: " << ex.what () << endl;
  }

  std::cout << "ae: init done" << std::endl;
}

void AstroidExtension::reader () {
  cout << "ae: reader thread: started." << endl;

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
          cout << "ae: " << m.msg () << endl;
        }
        break;

      case AeProtocol::MessageTypes::Mark:
        {
          AstroidMessages::Mark m;
          m.ParseFromString (buffer);
          handle_mark (m);
        }
        break;

      default:
        break; // unknown message
    }
  }
}

void AstroidExtension::handle_mark (AstroidMessages::Mark &m) {
  GError *err;
  ustring mid = "message_" + m.mid();

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);

  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (e);

  /* set class  */
  if (m.marked()) {
    webkit_dom_dom_token_list_add (class_list, (err = NULL, &err), "marked");
  } else {
    webkit_dom_dom_token_list_remove (class_list, (err = NULL, &err), "marked");
  }

  g_object_unref (class_list);
  g_object_unref (e);
  g_object_unref (d);
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

