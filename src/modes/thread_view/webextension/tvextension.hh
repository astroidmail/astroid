# pragma once

# include <webkit2/webkit-web-extension.h>
# include <gmodule.h>
# include <gtkmm.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <thread>
# include <mutex>

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
    ~AstroidExtension ();

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
    std::mutex      m_ostream;

    std::thread reader_t;
    void        reader ();
    bool        run = true;
    refptr<Gio::Cancellable> reader_cancel;
    void        ack (bool success);

    WebKitDOMNode * container;

    void handle_page (AstroidMessages::Page &s);

    /* state */
    bool edit_mode = false;
    AstroidMessages::State state;
    void handle_state (AstroidMessages::State &m);
    void clear_messages (AstroidMessages::ClearMessage &c);

    void handle_update_indent (AstroidMessages::Indent &i);
    void set_indent (bool);
    bool indent_messages;

    void handle_mark (AstroidMessages::Mark &m);
    void handle_hidden (AstroidMessages::Hidden &m);
    bool is_hidden (ustring);
    void set_hidden (ustring, bool);

    void add_message (AstroidMessages::Message &m);
    void remove_message (AstroidMessages::Message &m);
    void update_message (AstroidMessages::Message &m);

    void set_message_html (AstroidMessages::Message m,
        WebKitDOMHTMLElement * div_message);

    void create_message_part_html (
        const AstroidMessages::Message &m,
        const AstroidMessages::Message::Chunk &c,
        WebKitDOMHTMLElement * span_body);

    void create_body_part (
        /* const AstroidMessages::Message &message, */
        const AstroidMessages::Message::Chunk &c,
        WebKitDOMHTMLElement * span_body);

    void create_sibling_part (
        /* const AstroidMessages::Message &message, */
        const AstroidMessages::Message::Chunk &c,
        WebKitDOMHTMLElement * span_body);

    void insert_mime_messages (AstroidMessages::Message &m,
        WebKitDOMHTMLElement * div_message);
    void insert_attachments (AstroidMessages::Message &m,
        WebKitDOMHTMLElement * div_message);

    void message_render_tags (AstroidMessages::Message &m,
        WebKitDOMHTMLElement * div_message);
    void message_update_css_tags (AstroidMessages::Message &m,
        WebKitDOMHTMLElement * div_message);

    refptr<Gdk::Pixbuf> marked_icon;
    refptr<Gdk::Pixbuf> attachment_icon;

    static const int ATTACHMENT_ICON_WIDTH = 35;

    void load_marked_icon (WebKitDOMHTMLElement * div_message);
    void set_attachment_icon (WebKitDOMHTMLElement * div_message);

    /* headers */
    void insert_header_date (ustring &header, AstroidMessages::Message);
    void insert_header_address (ustring &header, ustring title, AstroidMessages::Address a, bool important);
    void insert_header_address_list (ustring &header, ustring title, AstroidMessages::AddressList al, bool important);
    void insert_header_row (ustring &header, ustring title, ustring value, bool important);
    ustring create_header_row (ustring title, ustring value, bool important, bool escape, bool noprint = false);

    /* warning and info */
    void set_warning (AstroidMessages::Info &);
    void hide_warning (AstroidMessages::Info &);
    void set_info (AstroidMessages::Info &);
    void hide_info (AstroidMessages::Info &);

    /* focus and scrolling */
    ustring focused_message = "";
    int     focused_element = -1;

    void handle_focus (AstroidMessages::Focus &m);
    void apply_focus (ustring mid, int element);

    void update_focus_to_view ();
    void scroll_to_element (ustring eid);

    void focus_next_element (bool force_change);
    void focus_previous_element (bool force_change);

    void focus_next_message ();
    void focus_previous_message (bool focus_top);

    const int BIG_JUMP = 150;
    const int STEP     = 35;

    void handle_navigate (AstroidMessages::Navigate &);
};

AstroidExtension * ext;

