# pragma once

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <thread>

# include "messages.pb.h"

# define refptr Glib::RefPtr
typedef Glib::ustring ustring;

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

    const int MAX_PREVIEW_LEN = 80;
    const int INDENT_PX       = 20;

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

    WebKitDOMNode * container;

    void handle_mark (AstroidMessages::Mark &m);
    void handle_hidden (AstroidMessages::Hidden &m);

    void add_message (AstroidMessages::Message &m);
    void set_message_html (AstroidMessages::Message m,
        WebKitDOMHTMLElement * div_message);
    void insert_mime_messages (AstroidMessages::Message m,
        WebKitDOMHTMLElement * div_message);
    void insert_attachments (AstroidMessages::Message m,
        WebKitDOMHTMLElement * div_message);
    void load_marked_icon (AstroidMessages::Message m,
        WebKitDOMHTMLElement * div_message);
    void set_attachment_icon (AstroidMessages::Message m,
        WebKitDOMHTMLElement * div_message);

    /* headers */
    void insert_header_date (ustring &header, AstroidMessages::Message);
    void insert_header_address (ustring &header, ustring title, AstroidMessages::Address a, bool important);
    void insert_header_address_list (ustring &header, ustring title, AstroidMessages::AddressList al, bool important);
    void insert_header_row (ustring &header, ustring title, ustring value, bool important);
    ustring create_header_row (ustring title, ustring value, bool important, bool escape, bool noprint);
    ustring header_row_value (ustring value, bool escape);

    void set_warning (AstroidMessages::Message, ustring);
    void set_error (AstroidMessages::Message, ustring);

};

AstroidExtension * ext;

