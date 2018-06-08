# include <iostream>
# include <thread>
# include <functional>

# include "tvextension.hh"

# include <webkit2/webkit-web-extension.h>

# include <gmodule.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <gtkmm.h>


# include "modes/thread_view/webextension/ae_protocol.hh"
# include "modes/thread_view/webextension/dom_utils.hh"
# include "utils/ustring_utils.hh"
# include "messages.pb.h"


using std::cout;
using std::endl;
using std::flush;

using namespace Astroid;

extern "C" {/*{{{*/

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

  /* IMPORTANT: We assume that there will only be one extension instance
   * per web page. That means that there can only be one page in each web view,
   * and each web view much use its own process. */

  ext = new AstroidExtension (extension, pipes);

  g_signal_connect (extension, "page-created",
      G_CALLBACK (web_page_created_callback),
      NULL);

}

}/*}}}*/

AstroidExtension::AstroidExtension (WebKitWebExtension * e,
    gpointer gaddr) {
  extension = e;

  std::cout << "ae: inititalize" << std::endl;

  Glib::init ();
  Gtk::Main::init_gtkmm_internals ();
  Gio::init ();

  /* load attachment icon */
  Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
  attachment_icon = theme->load_icon (
      "mail-attachment-symbolic",
      ATTACHMENT_ICON_WIDTH,
      Gtk::ICON_LOOKUP_USE_BUILTIN );

  /* load marked icon */
  marked_icon = theme->load_icon (
      "object-select-symbolic",
      ATTACHMENT_ICON_WIDTH,
      Gtk::ICON_LOOKUP_USE_BUILTIN );

  /* retrieve socket address */
  gsize sz;
  const char * caddr = g_variant_get_string ((GVariant *) gaddr, &sz);
  std::cout << "addr: " << caddr << std::endl;

  refptr<Gio::UnixSocketAddress> addr = Gio::UnixSocketAddress::create (caddr,
      Gio::UNIX_SOCKET_ADDRESS_ABSTRACT);

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

AstroidExtension::~AstroidExtension () {
  /* stop reader thread */
  run = false;
  if (reader_cancel)
    reader_cancel->cancel ();
  reader_t.join ();


  /* close connection */
  sock->close ();
}

void AstroidExtension::page_created (WebKitWebExtension * /* extension */,
    WebKitWebPage * _page,
    gpointer /* user_data */) {

  page = _page;

  cout << "ae: page created." << endl;
}

void AstroidExtension::write (const ustring t) {
  AstroidMessages::Debug m;
  m.set_msg (t);

  AeProtocol::send_message (AeProtocol::MessageTypes::Debug, m, ostream, m_ostream);
}

void AstroidExtension::reader () {/*{{{*/
  cout << "ae: reader thread: started." << endl;

  write ("reader started.");

  while (run) {
    gsize read = 0;

    cout << "ae: reader waiting.." << endl;

    /* read size of message */
    gsize sz;
    try {
      read = istream->read ((char*)&sz, sizeof (sz), reader_cancel); // blocking
    } catch (Gio::Error &e) {
      cout << "ae: reader thread: " << e.what () << endl;
      return;
    }

    if (read != sizeof(sz)) break;;

    /* read message type */
    AeProtocol::MessageTypes mt;
    read = istream->read ((char*)&mt, sizeof (mt), reader_cancel);
    if (read != sizeof (mt)) break;

    /* read message */
    gchar buffer[sz + 1]; buffer[sz] = '\0'; // TODO: set max buffer size
    bool s = istream->read_all (buffer, sz, read, reader_cancel);

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
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_mark), m));
        }
        break;

      case AeProtocol::MessageTypes::Hidden:
        {
          AstroidMessages::Hidden m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_hidden), m));
        }
        break;

      case AeProtocol::MessageTypes::Focus:
        {
          AstroidMessages::Focus m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_focus), m));
        }
        break;

      case AeProtocol::MessageTypes::State:
        {
          AstroidMessages::State m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_state), m));
        }
        break;

      case AeProtocol::MessageTypes::Page:
        {
          AstroidMessages::Page s;
          s.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_page), s));
        }
        break;

      case AeProtocol::MessageTypes::ClearMessages:
        {
          AstroidMessages::ClearMessage m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::clear_messages), m));
        }
        break;

      case AeProtocol::MessageTypes::AddMessage:
        {
          AstroidMessages::Message m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::add_message), m));
        }
        break;

      case AeProtocol::MessageTypes::UpdateMessage:
        {
          AstroidMessages::Message m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::update_message), m));
        }
        break;

      case AeProtocol::MessageTypes::RemoveMessage:
        {
          AstroidMessages::Message m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::remove_message), m));
        }
        break;

      case AeProtocol::MessageTypes::Info:
        {
          AstroidMessages::Info m;
          m.ParseFromString (buffer);

          if (m.warning ()) {
            Glib::signal_idle().connect_once (
                sigc::bind (
                  sigc::mem_fun(*this, &AstroidExtension::set_warning), m));
          } else {
            Glib::signal_idle().connect_once (
                sigc::bind (
                  sigc::mem_fun(*this, &AstroidExtension::set_info), m));
          }
        }
        break;

      case AeProtocol::MessageTypes::Navigate:
        {
          AstroidMessages::Navigate m;
          m.ParseFromString (buffer);
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_navigate), m));
        }
        break;

      default:
        break; // unknown message
    }
  }

  cout << "ae: reader thread exit." << endl;
}/*}}}*/

void AstroidExtension::handle_page (AstroidMessages::Page &s) {/*{{{*/
  GError *err = NULL;
  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);

  /* load html */
  cout << "ae: loading html.." << endl;

  WebKitDOMElement * he = webkit_dom_document_create_element (d, "HTML", (err = NULL, &err));
  webkit_dom_element_set_outer_html (he, s.html ().c_str (), (err = NULL, &err));

  webkit_dom_document_set_body (d, WEBKIT_DOM_HTML_ELEMENT(he), (err = NULL, &err));

  /* load css style */
  cout << "ae: loading stylesheet.." << flush;;
  WebKitDOMElement  *e = webkit_dom_document_create_element (d, "STYLE", (err = NULL, &err));

  WebKitDOMText *t = webkit_dom_document_create_text_node
    (d, s.css().c_str());

  webkit_dom_node_append_child (WEBKIT_DOM_NODE(e), WEBKIT_DOM_NODE(t), (err = NULL, &err));

  WebKitDOMHTMLHeadElement * head = webkit_dom_document_get_head (d);
  webkit_dom_node_append_child (WEBKIT_DOM_NODE(head), WEBKIT_DOM_NODE(e), (err = NULL, &err));
  cout << "done" << endl;

  g_object_unref (he);
  g_object_unref (head);
  g_object_unref (t);
  g_object_unref (e);
  g_object_unref (d);
}/*}}}*/

void AstroidExtension::handle_state (AstroidMessages::State &s) {/*{{{*/
  cout << "ae: got state." << endl;
  state = s;
  edit_mode = state.edit_mode ();
}/*}}}*/

void AstroidExtension::clear_messages (AstroidMessages::ClearMessage &) {
  cout << "ae: clearing all messages." << endl;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * container = DomUtils::get_by_id (d, "message_container");

  GError *err = NULL;

  webkit_dom_element_set_inner_html (container, "<span id=\"placeholder\"></span>", (err = NULL, &err));

  g_object_unref (container);
  g_object_unref (d);
}

// Message generation {{{
void AstroidExtension::add_message (AstroidMessages::Message &m) {
  cout << "ae: adding message: " << m.mid () << endl;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);

  /* there seems to be a bug in webkit2 where get_element_by_id() is unable to
   * find the DIV by id
   *
   * WebKitDOMElement * container = webkit_dom_document_get_element_by_id (d, "message_container");
   */

  WebKitDOMElement * container = DomUtils::get_by_id (d, "message_container");

  ustring div_id = "message_" + m.mid();

  WebKitDOMNode * insert_before = webkit_dom_node_get_last_child (
      WEBKIT_DOM_NODE(container));

  WebKitDOMHTMLElement * div_message = DomUtils::make_message_div (d);

  GError * err = NULL;
  webkit_dom_element_set_id (WEBKIT_DOM_ELEMENT (div_message), div_id.c_str());

  /* insert message div */
  webkit_dom_node_insert_before (WEBKIT_DOM_NODE(container),
      WEBKIT_DOM_NODE(div_message),
      insert_before,
      (err = NULL, &err));

  set_message_html (m, div_message);

  /* insert mime messages */
  if (!m.missing_content()) {
    insert_mime_messages (m, div_message);
  }

  /* insert attachments */
  if (!m.missing_content()) {
    insert_attachments (m, div_message);
  }

  /* marked */
  load_marked_icon (div_message);


  g_object_unref (insert_before);
  g_object_unref (div_message);
  g_object_unref (container);
  g_object_unref (d);

  cout << "ae: message added." << endl;

  apply_focus (focused_message, focused_element); // in case we got focus before message was added.
}

void AstroidExtension::remove_message (AstroidMessages::Message &m) {

}

void AstroidExtension::update_message (AstroidMessages::Message &m) {

}

/* main message generation  */
void AstroidExtension::set_message_html (
    AstroidMessages::Message m,
    WebKitDOMHTMLElement * div_message)
{
  GError *err;

  /* load message into div */
  ustring header;
  WebKitDOMHTMLElement * div_email_container =
    DomUtils::select (WEBKIT_DOM_NODE(div_message),  ".email_container");

  /* build header */
  insert_header_address (header, "From", m.sender(), true);

  if (m.reply_to().email().size () > 0) {
    if (m.reply_to().email() != m.sender().email())
      insert_header_address (header, "Reply-To", m.reply_to(), false);
  }

  insert_header_address_list (header, "To", m.to(), false);

  if (m.cc().addresses().size () > 0) {
    insert_header_address_list (header, "Cc", m.cc(), false);
  }

  if (m.bcc().addresses().size () > 0) {
    insert_header_address_list (header, "Bcc", m.bcc(), false);
  }

  insert_header_date (header, m);

  if (m.subject().size() > 0) {
    insert_header_row (header, "Subject", m.subject(), false);

    WebKitDOMHTMLElement * subject = DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".header_container .subject");

    ustring s = Glib::Markup::escape_text(m.subject());
    if (static_cast<int>(s.size()) > MAX_PREVIEW_LEN)
      s = s.substr(0, MAX_PREVIEW_LEN - 3) + "...";

    webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT (subject), s.c_str(), (err = NULL, &err));

    g_object_unref (subject);
  }

  /* avatar */
  if (!m.gravatar().empty ()) {
    WebKitDOMHTMLImageElement * av = WEBKIT_DOM_HTML_IMAGE_ELEMENT (
        DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".avatar" ));

    webkit_dom_html_image_element_set_src (av, m.gravatar().c_str());

    g_object_unref (av);
  }

  /* insert header html*/
  WebKitDOMHTMLElement * table_header =
    DomUtils::select (WEBKIT_DOM_NODE(div_email_container),
        ".header_container .header" );

  if (!edit_mode && m.tags().size () > 0) {
    header += create_header_row ("Tags", "", false, false, true);
  }

  webkit_dom_element_set_inner_html (
      WEBKIT_DOM_ELEMENT(table_header),
      header.c_str(),
      (err = NULL, &err));

  if (!edit_mode && m.tags().size () > 0) {
    message_render_tags (m, WEBKIT_DOM_HTML_ELEMENT(div_message));
    message_update_css_tags (m, WEBKIT_DOM_HTML_ELEMENT(div_message));
  }

  /* if message is missing body, set warning and don't add any content */
  WebKitDOMHTMLElement * span_body =
    DomUtils::select (WEBKIT_DOM_NODE(div_email_container),
        ".body" );

  WebKitDOMHTMLElement * preview =
    DomUtils::select (WEBKIT_DOM_NODE(div_email_container),
        ".header_container .preview" );

  if (m.missing_content()) {
    /* set preview */
    webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(preview), "<i>Message content is missing.</i>", (err = NULL, &err));

    /* set warning */
    AstroidMessages::Info i;
    i.set_mid (m.mid());
    i.set_set (true);
    i.set_txt ("The message file is missing, only fields cached in the notmuch database are shown. Most likely your database is out of sync.");

    set_warning (i);

    /* add an explanation to the body */
    GError *err;

    WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
    WebKitDOMHTMLElement * body_container =
      DomUtils::clone_get_by_id (d, "body_template");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (body_container),
        "id");

    webkit_dom_element_set_inner_html (
        WEBKIT_DOM_ELEMENT(body_container),
        "<i>Message content is missing.</i>",
        (err = NULL, &err));

    webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
        WEBKIT_DOM_NODE (body_container), (err = NULL, &err));

    g_object_unref (body_container);
    g_object_unref (d);

  } else {

    /* build message body */
    create_message_part_html (m, m.root(), span_body);

    /* preview */
    webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(preview), m.preview().c_str(), (err = NULL, &err));
  }

  g_object_unref (preview);
  g_object_unref (span_body);
  g_object_unref (table_header);
} //

void AstroidExtension::message_render_tags (AstroidMessages::Message &m,
    WebKitDOMHTMLElement * div_message) {

  if (edit_mode) return;

  GError *err;

  WebKitDOMHTMLElement * tags = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".header_container .tags");

  webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(tags), m.tag_string().c_str (), (err = NULL, &err));

  g_object_unref (tags);

  tags = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".header_container .header div#Tags .value");

  webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(tags), m.tag_string().c_str (), (err = NULL, &err));

  g_object_unref (tags);
}

void AstroidExtension::message_update_css_tags (AstroidMessages::Message &m,
    WebKitDOMHTMLElement * div_message) {
  if (edit_mode) return;

  /* check for tag changes that control display */
  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(div_message));

  /* patches may be rendered somewhat differently */
  DomUtils::switch_class (class_list, "patch", m.patch ());

  /* message subject deviates from thread subject */
  DomUtils::switch_class (class_list, "different_subject", m.different_subject ());

  /* reset notmuch tags */
  for (unsigned int i = 0; i < webkit_dom_dom_token_list_get_length (class_list); i++)
  {
    const char * _t = webkit_dom_dom_token_list_item (class_list, i);
    ustring t (_t);

    if (t.find ("nm-", 0) != std::string::npos) {
      DomUtils::switch_class (class_list, t, false);
    }
  }

  for (ustring t : m.tags()) {
    t = UstringUtils::replace (t, "/", "-");
    t = UstringUtils::replace (t, ".", "-");
    t = Glib::Markup::escape_text (t);

    t = "nm-" + t;
    DomUtils::switch_class (class_list, t, true);
  }

  g_object_unref (class_list);
}

void AstroidExtension::create_message_part_html (
    const AstroidMessages::Message &message,
    const AstroidMessages::Message::Chunk &c,
    WebKitDOMHTMLElement * span_body)
{

  ustring mime_type = c.mime_type ();

  cout << "create message part: " << c.id() << " (siblings: " << c.sibling() << ") (kids: " << c.kids().size() << ")" <<
    " (attachment: " << c.attachment() << ")" << " (viewable: " << c.viewable() << ")" << " (mimetype: " << mime_type << ")" << endl;

  if (c.use()) {
    if (c.viewable() && c.preferred()) {
      create_body_part (c, span_body);
    } else if (c.viewable()) {
      create_sibling_part (c, span_body);
    }

    for (auto &k: c.kids()) {
      create_message_part_html (message, k, span_body);
    }
  } else {
    create_sibling_part (c, span_body);
  }
}


void AstroidExtension::create_body_part (
    /* const AstroidMessages::Message &message, */
    const AstroidMessages::Message::Chunk &c,
    WebKitDOMHTMLElement * span_body)
{
  // <span id="body_template" class="body_part"></span>

  cout << "create body part: " << c.id() << endl;

  GError *err;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMHTMLElement * body_container =
    DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#body_template");

  webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (body_container),
      "id");

  ustring body = c.content();

  /*
   * TODO:
  if (code_is_on) {
    if (c.patch ()) {
      LOG (debug) << "tv: message is patch, syntax highlighting.";
      body.insert (0, code_start_tag);
      body.insert (body.length()-1, code_stop_tag);

    } else {
      filter_code_tags (body);
    }
  }
  */

  webkit_dom_element_set_inner_html (
      WEBKIT_DOM_ELEMENT(body_container),
      body.c_str(),
      (err = NULL, &err));

  /* check encryption */
  //
  //  <div id="encrypt_template" class=encrypt_container">
  //      <div class="message"></div>
  //  </div>
#if 0
  if (c->is_encrypted() || c->is_signed()) {
    WebKitDOMHTMLElement * encrypt_container =
      DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#encrypt_template");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (encrypt_container),
        "id");

    // add to message state
    MessageState::Element e (MessageState::ElementType::Encryption, c->id);
    state[message].elements.push_back (e);
    LOG (debug) << "tv: added encrypt: " << c->id;

    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (encrypt_container),
      "id", e.element_id().c_str(),
      (err = NULL, &err));

    WebKitDOMDOMTokenList * class_list_e =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(encrypt_container));



    ustring content = "";

    ustring sign_string = "";
    ustring enc_string  = "";

    vector<ustring> all_sig_errors;

    if (c->issigned) {

      refptr<Crypto> cr = c->crypt;

      if (cr->verified) {
        sign_string += "<span class=\"header\">Signature verification succeeded.</span>";
      } else {
        sign_string += "<span class=\"header\">Signature verification failed!</span>";
      }

      for (int i = 0; i < g_mime_signature_list_length (cr->slist); i++) {
        GMimeSignature * s = g_mime_signature_list_get_signature (cr->slist, i);
        GMimeCertificate * ce = NULL;
        if (s) ce = g_mime_signature_get_certificate (s);

        ustring nm, em, ky;
        ustring gd = "";
        ustring err = "";
        vector<ustring> sig_errors;

        if (ce) {
          const char * c = NULL;
          nm = (c = g_mime_certificate_get_name (ce), c ? c : "");
          em = (c = g_mime_certificate_get_email (ce), c ? c : "");
          ky = (c = g_mime_certificate_get_key_id (ce), c ? c : "");


# if (GMIME_MAJOR_VERSION < 3)
          switch (g_mime_signature_get_status (s)) {
            case GMIME_SIGNATURE_STATUS_GOOD:
              gd = "Good";
              break;

            case GMIME_SIGNATURE_STATUS_BAD:
              gd = "Bad";
              // fall through

            case GMIME_SIGNATURE_STATUS_ERROR:
              if (gd.empty ()) gd = "Erroneous";

              GMimeSignatureError e = g_mime_signature_get_errors (s);
              if (e & GMIME_SIGNATURE_ERROR_EXPSIG)
                sig_errors.push_back ("expired");
              if (e & GMIME_SIGNATURE_ERROR_NO_PUBKEY)
                sig_errors.push_back ("no-pub-key");
              if (e & GMIME_SIGNATURE_ERROR_EXPKEYSIG)
                sig_errors.push_back ("expired-key-sig");
              if (e & GMIME_SIGNATURE_ERROR_REVKEYSIG)
                sig_errors.push_back ("revoked-key-sig");
              if (e & GMIME_SIGNATURE_ERROR_UNSUPP_ALGO)
                sig_errors.push_back ("unsupported-algo");
              if (!sig_errors.empty ()) {
                err = "[Error: " + VectorUtils::concat (sig_errors, ",") + "]";
              }
              break;
# else
          GMimeSignatureStatus stat = g_mime_signature_get_status (s);
          if (g_mime_signature_status_good (stat)) {
              gd = "Good";
          } else if (g_mime_signature_status_bad (stat) || g_mime_signature_status_error (stat)) {

            if (g_mime_signature_status_bad (stat)) gd = "Bad";
            else gd = "Erroneous";

            if (stat & GMIME_SIGNATURE_STATUS_KEY_REVOKED) sig_errors.push_back ("revoked-key");
            if (stat & GMIME_SIGNATURE_STATUS_KEY_EXPIRED) sig_errors.push_back ("expired-key");
            if (stat & GMIME_SIGNATURE_STATUS_SIG_EXPIRED) sig_errors.push_back ("expired-sig");
            if (stat & GMIME_SIGNATURE_STATUS_KEY_MISSING) sig_errors.push_back ("key-missing");
            if (stat & GMIME_SIGNATURE_STATUS_CRL_MISSING) sig_errors.push_back ("crl-missing");
            if (stat & GMIME_SIGNATURE_STATUS_CRL_TOO_OLD) sig_errors.push_back ("crl-too-old");
            if (stat & GMIME_SIGNATURE_STATUS_BAD_POLICY)  sig_errors.push_back ("bad-policy");
            if (stat & GMIME_SIGNATURE_STATUS_SYS_ERROR)   sig_errors.push_back ("sys-error");
            if (stat & GMIME_SIGNATURE_STATUS_TOFU_CONFLICT) sig_errors.push_back ("tofu-conflict");

            if (!sig_errors.empty ()) {
              err = "[Error: " + VectorUtils::concat (sig_errors, ",") + "]";
            }
# endif
          }
        } else {
          err = "[Error: Could not get certificate]";
        }

# if (GMIME_MAJOR_VERSION < 3)
        GMimeCertificateTrust t = g_mime_certificate_get_trust (ce);
        ustring trust = "";
        switch (t) {
          case GMIME_CERTIFICATE_TRUST_NONE: trust = "none"; break;
          case GMIME_CERTIFICATE_TRUST_NEVER: trust = "never"; break;
          case GMIME_CERTIFICATE_TRUST_UNDEFINED: trust = "undefined"; break;
          case GMIME_CERTIFICATE_TRUST_MARGINAL: trust = "marginal"; break;
          case GMIME_CERTIFICATE_TRUST_FULLY: trust = "fully"; break;
          case GMIME_CERTIFICATE_TRUST_ULTIMATE: trust = "ultimate"; break;
        }
# else
        GMimeTrust t = g_mime_certificate_get_trust (ce);
        ustring trust = "";
        switch (t) {
          case GMIME_TRUST_UNKNOWN: trust = "unknown"; break;
          case GMIME_TRUST_UNDEFINED: trust = "undefined"; break;
          case GMIME_TRUST_NEVER: trust = "never"; break;
          case GMIME_TRUST_MARGINAL: trust = "marginal"; break;
          case GMIME_TRUST_FULL: trust = "full"; break;
          case GMIME_TRUST_ULTIMATE: trust = "ultimate"; break;
        }
# endif


        sign_string += ustring::compose (
            "<br />%1 signature from: %2 (%3) [0x%4] [trust: %5] %6",
            gd, nm, em, ky, trust, err);


        all_sig_errors.insert (all_sig_errors.end(), sig_errors.begin (), sig_errors.end ());
      }
    }

    if (c->isencrypted) {
      refptr<Crypto> cr = c->crypt;

      if (c->issigned) enc_string = "<span class=\"header\">Signed and Encrypted.</span>";
      else             enc_string = "<span class=\"header\">Encrypted.</span>";

      if (cr->decrypted) {

        GMimeCertificateList * rlist = cr->rlist;
        for (int i = 0; i < g_mime_certificate_list_length (rlist); i++) {

          GMimeCertificate * ce = g_mime_certificate_list_get_certificate (rlist, i);

          const char * c = NULL;
          ustring fp = (c = g_mime_certificate_get_fingerprint (ce), c ? c : "");
          ustring nm = (c = g_mime_certificate_get_name (ce), c ? c : "");
          ustring em = (c = g_mime_certificate_get_email (ce), c ? c : "");
          ustring ky = (c = g_mime_certificate_get_key_id (ce), c ? c : "");

          enc_string += ustring::compose ("<br /> Encrypted for: %1 (%2) [0x%3]",
              nm, em, ky);
        }

        if (c->issigned) enc_string += "<br /><br />";

      } else {
        enc_string += "Encrypted: Failed decryption.";
      }

    }

    content = enc_string + sign_string;

    WebKitDOMHTMLElement * message_cont =
      DomUtils::select (WEBKIT_DOM_NODE (encrypt_container), ".message");

    webkit_dom_element_set_inner_html (
        WEBKIT_DOM_ELEMENT(message_cont),
        content.c_str(),
        (err = NULL, &err));


    webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
        WEBKIT_DOM_NODE (encrypt_container), (err = NULL, &err));

    /* add encryption tag to encrypted part */
    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(body_container));

    if (c->isencrypted) {
      webkit_dom_dom_token_list_add (class_list_e, "encrypted",
          (err = NULL, &err));
      webkit_dom_dom_token_list_add (class_list, "encrypted",
          (err = NULL, &err));

      if (!c->crypt->decrypted) {
        webkit_dom_dom_token_list_add (class_list_e, "decrypt_failed",
            (err = NULL, &err));
        webkit_dom_dom_token_list_add (class_list, "decrypt_failed",
            (err = NULL, &err));
      }
    }

    if (c->issigned) {
      webkit_dom_dom_token_list_add (class_list_e, "signed",
          (err = NULL, &err));

      webkit_dom_dom_token_list_add (class_list, "signed",
          (err = NULL, &err));

      if (!c->crypt->verified) {
        webkit_dom_dom_token_list_add (class_list_e, "verify_failed",
            (err = NULL, &err));
        webkit_dom_dom_token_list_add (class_list, "verify_failed",
            (err = NULL, &err));

        /* add specific errors */
        std::sort (all_sig_errors.begin (), all_sig_errors.end ());
        all_sig_errors.erase (unique (all_sig_errors.begin (), all_sig_errors.end ()), all_sig_errors.end ());

        for (ustring & e : all_sig_errors) {
          webkit_dom_dom_token_list_add (class_list_e, e.c_str (),
              (err = NULL, &err));
          webkit_dom_dom_token_list_add (class_list, e.c_str (),
              (err = NULL, &err));
        }
      }
    }

    g_object_unref (class_list);
    g_object_unref (class_list_e);
    g_object_unref (encrypt_container);
    g_object_unref (message_cont);

  }
# endif

  webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
      WEBKIT_DOM_NODE (body_container), (err = NULL, &err));

  g_object_unref (body_container);
  g_object_unref (d);
}

void AstroidExtension::create_sibling_part (
    /* const AstroidMessages::Message &message, */
    const AstroidMessages::Message::Chunk &sibling,
    WebKitDOMHTMLElement * span_body) {

  cout << "create sibling part: " << sibling.id () << endl;
  //
  //  <div id="sibling_template" class=sibling_container">
  //      <div class="message"></div>
  //  </div>

  GError *err;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMHTMLElement * sibling_container =
    DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#sibling_template");

  webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (sibling_container),
      "id");

  webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (sibling_container),
    "id", sibling.sid().c_str(),
    (err = NULL, &err));

  ustring content = ustring::compose ("Alternative part (type: %1) - potentially sketchy.",
      Glib::Markup::escape_text(sibling.mime_type ()),
      sibling.sid());

  WebKitDOMHTMLElement * message_cont =
    DomUtils::select (WEBKIT_DOM_NODE (sibling_container), ".message");

  webkit_dom_element_set_inner_html (
      WEBKIT_DOM_ELEMENT(message_cont),
      content.c_str(),
      (err = NULL, &err));

  webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
      WEBKIT_DOM_NODE (sibling_container), (err = NULL, &err));

  g_object_unref (message_cont);
  g_object_unref (sibling_container);
  g_object_unref (d);
} //

void AstroidExtension::insert_mime_messages (
    AstroidMessages::Message &m,
    WebKitDOMHTMLElement * div_message)
{
  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMHTMLElement * div_email_container =
    DomUtils::select (WEBKIT_DOM_NODE(div_message), "div.email_container");

  WebKitDOMHTMLElement * span_body =
    DomUtils::select (WEBKIT_DOM_NODE(div_email_container), ".body");

  for (auto &c : m.mime_messages ()) {
    cout << "create mime message part: " << c.id() << endl;
    //
    //  <div id="mime_template" class=mime_container">
    //      <div class="top_border"></div>
    //      <div class="message"></div>
    //  </div>

    GError *err;

    WebKitDOMHTMLElement * mime_container =
      DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#mime_template");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (mime_container),
        "id");

    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (mime_container),
      "id", c.sid().c_str(),
      (err = NULL, &err));

    ustring content = ustring::compose ("MIME message (subject: %1, size: %2 B) - potentially sketchy.",
        Glib::Markup::escape_text(c.filename ()),
        c.human_size (),
        c.sid ());

    WebKitDOMHTMLElement * message_cont =
      DomUtils::select (WEBKIT_DOM_NODE (mime_container), ".message");

    webkit_dom_element_set_inner_html (
        WEBKIT_DOM_ELEMENT(message_cont),
        content.c_str(),
        (err = NULL, &err));


    webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
        WEBKIT_DOM_NODE (mime_container), (err = NULL, &err));

    g_object_unref (message_cont);
    g_object_unref (mime_container);

  }

  g_object_unref (span_body);
  g_object_unref (div_email_container);
  g_object_unref (d);
}

void AstroidExtension::insert_attachments (
    AstroidMessages::Message &m,
    WebKitDOMHTMLElement * div_message)
{
  // <div class="attachment_container">
  //     <div class="top_border"></div>
  //     <table class="attachment" data-attachment-id="">
  //         <tr>
  //             <td class="preview">
  //                 <img src="" />
  //             </td>
  //             <td class="info">
  //                 <div class="filename"></div>
  //                 <div class="filesize"></div>
  //             </td>
  //         </tr>
  //     </table>
  // </div>

  GError *err;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMHTMLElement * attachment_container =
    DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#attachment_template");
  WebKitDOMHTMLElement * attachment_template =
    DomUtils::select (WEBKIT_DOM_NODE(attachment_container), ".attachment");

  webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (attachment_container),
      "id");
  webkit_dom_node_remove_child (WEBKIT_DOM_NODE (attachment_container),
      WEBKIT_DOM_NODE(attachment_template), (err = NULL, &err));

  int attachments = 0;

  /* generate an attachment table for each attachment */
  for (auto &c : m.attachments ()) {
    WebKitDOMHTMLElement * attachment_table =
      DomUtils::clone_node (WEBKIT_DOM_NODE (attachment_template));

    attachments++;

    WebKitDOMHTMLElement * info_fname =
      DomUtils::select (WEBKIT_DOM_NODE (attachment_table), ".info .filename");

    ustring fname = c.filename ();
    if (fname.size () == 0) {
      fname = "Unnamed attachment";
    }

    fname = Glib::Markup::escape_text (fname);

    webkit_dom_html_element_set_inner_text (info_fname, fname.c_str(), (err = NULL, &err));

    WebKitDOMHTMLElement * info_fsize =
      DomUtils::select (WEBKIT_DOM_NODE (attachment_table), ".info .filesize");

    webkit_dom_html_element_set_inner_text (info_fsize, c.human_size().c_str(), (err = NULL, &err));


    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (attachment_table),
      "data-attachment-id", c.sid().c_str(),
      (err = NULL, &err));
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (attachment_table),
      "id", c.sid().c_str(),
      (err = NULL, &err));

    // set image
    WebKitDOMHTMLImageElement * img =
      WEBKIT_DOM_HTML_IMAGE_ELEMENT(
      DomUtils::select (WEBKIT_DOM_NODE (attachment_table), ".preview img"));

    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
        c.thumbnail().c_str(), &err);

    // add the attachment table
    webkit_dom_node_append_child (WEBKIT_DOM_NODE (attachment_container),
        WEBKIT_DOM_NODE (attachment_table), (err = NULL, &err));


    /* if (c->issigned || c->isencrypted) { */
    /*   /1* add encryption or signed tag to attachment *1/ */
    /*   WebKitDOMDOMTokenList * class_list = */
    /*     webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(attachment_table)); */

    /*   if (c->isencrypted) { */
    /*     webkit_dom_dom_token_list_add (class_list, "encrypted", */
    /*         (err = NULL, &err)); */
    /*   } */

    /*   if (c->issigned) { */
    /*     webkit_dom_dom_token_list_add (class_list, "signed", */
    /*         (err = NULL, &err)); */
    /*   } */

    /*   g_object_unref (class_list); */
    /* } */

    g_object_unref (img);
    g_object_unref (info_fname);
    g_object_unref (info_fsize);
    g_object_unref (attachment_table);

  }

  if (attachments > 0) {
    webkit_dom_node_append_child (WEBKIT_DOM_NODE (div_message),
        WEBKIT_DOM_NODE (attachment_container), (err = NULL, &err));
  }

  g_object_unref (attachment_template);
  g_object_unref (attachment_container);
  g_object_unref (d);


  if (attachments > 0)
    set_attachment_icon (div_message);
}

void AstroidExtension::set_attachment_icon (
    WebKitDOMHTMLElement * div_message)
{
  GError *err;

  WebKitDOMHTMLElement * attachment_icon_img = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".attachment.icon.first");

  gchar * content;
  gsize   content_size;
  attachment_icon->save_to_buffer (content, content_size, "png");
  ustring image_content_type = "image/png";

  WebKitDOMHTMLImageElement *img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (attachment_icon_img);

  err = NULL;
  webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
      DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str(), &err);

  g_object_unref (attachment_icon_img);

  attachment_icon_img = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".attachment.icon.sec");
  img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (attachment_icon_img);

  err = NULL;
  webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
      DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str(), &err);

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(div_message));

  DomUtils::switch_class (class_list, "attachment", true);

  g_object_unref (class_list);
  g_object_unref (attachment_icon_img);
}

void AstroidExtension::load_marked_icon (WebKitDOMHTMLElement * div_message) {

  WebKitDOMHTMLElement * marked_icon_img = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".marked.icon.first");

  gchar * content;
  gsize   content_size;
  marked_icon->save_to_buffer (content, content_size, "png");
  ustring image_content_type = "image/png";

  WebKitDOMHTMLImageElement *img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (marked_icon_img);

  webkit_dom_html_image_element_set_src (img, DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str());

  g_object_unref (marked_icon_img);

  marked_icon_img = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".marked.icon.sec");
  img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (marked_icon_img);

  webkit_dom_html_image_element_set_src (img, DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str());

  g_object_unref (marked_icon_img);
}

/* headers  {{{ */
void AstroidExtension::insert_header_date (ustring & header, AstroidMessages::Message m)
{
  ustring value = ustring::compose (
              "<span class=\"hidden_only\">%1</span>"
              "<span class=\"not_hidden_only\">%2</span>",
              m.date_pretty (),
              m.date_verbose ());

  header += create_header_row ("Date", value, true, false);
}

void AstroidExtension::insert_header_address (
    ustring &header,
    ustring title,
    AstroidMessages::Address address,
    bool important) {

  AstroidMessages::AddressList al;
  AstroidMessages::Address * a = al.add_addresses ();
  a->set_name (address.name());
  a->set_full_address (address.full_address ());
  a->set_email (address.email ());

  insert_header_address_list (header, title, al, important);
}

void AstroidExtension::insert_header_address_list (
    ustring &header,
    ustring title,
    AstroidMessages::AddressList addresses,
    bool important) {

  ustring value;
  bool first = true;

  for (const AstroidMessages::Address address : addresses.addresses()) {
    if (address.full_address().size() > 0) {
      if (!first) {
        value += ", ";
      } else {
        first = false;
      }

      value +=
        ustring::compose ("<a href=\"mailto:%3\">%4%1%5 &lt;%2&gt;</a>",
          Glib::Markup::escape_text (address.name ()),
          Glib::Markup::escape_text (address.email ()),
          Glib::Markup::escape_text (address.full_address()),
          (important ? "<b>" : ""),
          (important ? "</b>" : "")
          );
    }
  }

  header += create_header_row (title, value, important, false, false);
}

void AstroidExtension::insert_header_row (
    ustring &header,
    ustring title,
    ustring value,
    bool important) {

  header += create_header_row (title, value, important, true, false);
}


ustring AstroidExtension::create_header_row (
    ustring title,
    ustring value,
    bool important,
    bool escape,
    bool noprint) {

  return ustring::compose (
      "<div class=\"field%1%2\" id=\"%3\">"
      "  <div class=\"title\">%3:</div>"
      "  <div class=\"value\">%4</div>"
      "</div>",
      (important ? " important" : ""),
      (noprint ? " noprint" : ""),
      Glib::Markup::escape_text (title),
      (escape ? Glib::Markup::escape_text (value) : value)
      );
}
/* headers end  }}} */

// }}}

/* warning and info {{{ */
void AstroidExtension::set_warning (AstroidMessages::Info &m) {
  if (!m.set ()) {
    hide_warning (m);
    return;
  }

  ustring mid = "message_" + m.mid();
  ustring txt = m.txt();

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  WebKitDOMHTMLElement * warning = DomUtils::select (
      WEBKIT_DOM_NODE (e),
      ".email_warning");

  GError * err;
  webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(warning), txt.c_str(), (err = NULL, &err));

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(warning));

  DomUtils::switch_class (class_list, "show", true);

  g_object_unref (class_list);
  g_object_unref (warning);
  g_object_unref (e);
  g_object_unref (d);
}

void AstroidExtension::hide_warning (AstroidMessages::Info &m)
{
  ustring mid = "message_" + m.mid();

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  WebKitDOMHTMLElement * warning = DomUtils::select (
      WEBKIT_DOM_NODE (e),
      ".email_warning");

  GError * err;
  webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(warning), "", (err = NULL, &err));

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(warning));

  DomUtils::switch_class (class_list, "show", false);

  g_object_unref (class_list);
  g_object_unref (warning);
  g_object_unref (e);
  g_object_unref (d);
}

void AstroidExtension::set_info (AstroidMessages::Info &m)
{
  if (!m.set ()) {
    hide_info (m);
    return;
  }

  ustring mid = "message_" + m.mid();
  ustring txt = m.txt();

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  WebKitDOMHTMLElement * info = DomUtils::select (
      WEBKIT_DOM_NODE (e),
      ".email_info");

  GError * err;
  webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(info), txt.c_str(), (err = NULL, &err));

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(info));

  DomUtils::switch_class (class_list, "show", true);

  g_object_unref (class_list);
  g_object_unref (info);
  g_object_unref (e);
  g_object_unref (d);
}

void AstroidExtension::hide_info (AstroidMessages::Info &m) {
  ustring mid = "message_" + m.mid();

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  WebKitDOMHTMLElement * info = DomUtils::select (
      WEBKIT_DOM_NODE (e),
      ".email_info");

  GError * err;
  webkit_dom_element_set_inner_html (WEBKIT_DOM_ELEMENT(info), "", (err = NULL, &err));

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(info));

  DomUtils::switch_class (class_list, "show", false);

  g_object_unref (class_list);
  g_object_unref (info);
  g_object_unref (e);
  g_object_unref (d);
}

/* }}} */

void AstroidExtension::handle_hidden (AstroidMessages::Hidden &msg) {/*{{{*/
  /* hide or show message */
  ustring div_id = "message_" + msg.mid();

  GError * err = NULL;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, div_id.c_str());

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

  if (msg.hidden ()) {
    webkit_dom_dom_token_list_toggle (class_list, "hide", msg.hidden (), &err );
  } else if (webkit_dom_dom_token_list_contains (class_list, "hide")) {
    webkit_dom_dom_token_list_toggle (class_list, "hide", false, &err );
  }

  g_object_unref (class_list);
  g_object_unref (e);
  g_object_unref (d);
}/*}}}*/

bool AstroidExtension::is_hidden (ustring mid) {
  ustring mmid = "message_" + mid;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mmid.c_str());

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (e);

  bool r = webkit_dom_dom_token_list_contains (class_list, "hide");

  g_object_unref (class_list);
  g_object_unref (e);
  g_object_unref (d);

  return r;
}

void AstroidExtension::handle_mark (AstroidMessages::Mark &m) {/*{{{*/
  ustring mid = "message_" + m.mid();

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);

  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (e);

  /* set class  */
  DomUtils::switch_class (class_list, "marked", m.marked());

  g_object_unref (class_list);
  g_object_unref (e);
  g_object_unref (d);
}/*}}}*/

void AstroidExtension::handle_focus (AstroidMessages::Focus &msg) {
  cout << "ae: focusing: " << msg.mid() << ": " << msg.element () << endl;

  apply_focus (msg.mid (), msg.element ());
}

void AstroidExtension::apply_focus (ustring mid, int element) {
  focused_message = mid;
  focused_element = element;

  if (focused_message.empty() || focused_element == -1) return;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);

  for (auto &m : state.messages()) {

    ustring _mid = "message_" + m.mid ();

    WebKitDOMElement * me = webkit_dom_document_get_element_by_id (d, _mid.c_str ());
    WebKitDOMDOMTokenList * class_list = webkit_dom_element_get_class_list (me);

    /* set class  */
    DomUtils::switch_class (class_list, "focused", m.mid () == mid);

    g_object_unref (class_list);

    int ei = 1;
    for (auto &e : m.elements ()) {
      if (e.id() < 0) continue; // all states contain an empty element at first.

      WebKitDOMElement * ee = webkit_dom_document_get_element_by_id (d, e.sid().c_str());
      WebKitDOMDOMTokenList * e_class_list =
        webkit_dom_element_get_class_list (ee);

      DomUtils::switch_class (e_class_list, "focused", (m.mid () == mid && ei == element));

      g_object_unref (e_class_list);
      g_object_unref (ee);

      ei++;
    }

    g_object_unref (me);
  }

  g_object_unref (d);

  /* send back focus to ThreadView */
  AstroidMessages::Focus fe;
  fe.set_mid (focused_message);
  fe.set_element (element);
  fe.set_focus (true);

  cout << "ae: sending focus event: " << fe.mid () << ", element: " << fe.element () << endl;

  AeProtocol::send_message (AeProtocol::MessageTypes::Focus, fe, ostream, m_ostream);
}

void AstroidExtension::update_focus_to_view () {
  /* check if currently focused message has gone out of focus
   * and update focus */

  /* loop through elements from the top and test whether the top
   * of it is within the view
   */

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);
  WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));

  double scrolled = webkit_dom_dom_window_get_scroll_y (w);
  double height   = webkit_dom_element_get_client_height (body);

  //LOG (debug) << "scrolled = " << scrolled << ", height = " << height;

  /* check currently focused message */
  bool take_next = false;

  /* need to apply_focus afterwards */
  bool redo_focus = false;

  /* take first if none focused */
  if (focused_message.empty ()) {
    focused_message = state.messages()[0].mid() ;
    redo_focus = true;
  }

  /* check if focused message is still visible */
  ustring mid = "message_" + focused_message;

  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

  double clientY = webkit_dom_element_get_offset_top (e);
  double clientH = webkit_dom_element_get_client_height (e);

  g_object_unref (e);

  //LOG (debug) << "y = " << clientY;
  //LOG (debug) << "h = " << clientH;

  // height = 0 if there is no paging: all messages are in view.
  if ((height == 0) || ( (clientY <= (scrolled + height)) && ((clientY + clientH) >= scrolled) )) {
    //LOG (debug) << "message: " << focused_message->date() << " still in view.";
  } else {
    //LOG (debug) << "message: " << focused_message->date() << " out of view.";
    take_next = true;
  }

  /* find first message that is in view and update
   * focused status */
  if (take_next) {
    int focused_position = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; }) - state.messages().begin ();
    int cur_position = 0;

    bool found = false;

    for (auto &m : state.messages()) {
      ustring mid = "message_" + m.mid();

      WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

      double clientY = webkit_dom_element_get_offset_top (e);
      double clientH = webkit_dom_element_get_client_height (e);

      // LOG (debug) << "y = " << clientY;
      // LOG (debug) << "h = " << clientH;

      /* search for the last message that is currently in view
       * if the focused message is now below / beyond the view.
       * otherwise, take first that is in view now. */

      if ((!found || cur_position < focused_position) &&
          ( (height == 0) || ((clientY <= (scrolled + height)) && ((clientY + clientH) >= scrolled)) ))
      {
        // LOG (debug) << "message: " << m->date() << " now in view.";

        if (found) redo_focus = true;
        found = true;
        focused_message = m.mid();
        focused_element = 0;

      } else {
        /* reset class */
        redo_focus = true;
      }

      g_object_unref (e);

      cur_position++;
    }

    g_object_unref (body);
    g_object_unref (w);
    g_object_unref (d);

    if (redo_focus) apply_focus (focused_message, focused_element);
  }

}

void AstroidExtension::focus_next_element (bool force_change) {
  ustring eid;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);
  WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));

  if (!is_hidden (focused_message) || edit_mode) {
    /* if the message is expanded, check if we should move focus
     * to the next element */

    auto s = *std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

    /* are there any more elements */
    if (focused_element < (s.elements().size()-1)) {
      /* check if the next element is in full view */

      auto next_e = s.elements()[focused_element + 1];

      eid = next_e.sid ();

      bool change_focus = force_change;

      if (!force_change) {
        WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());

        double scrolled = webkit_dom_dom_window_get_scroll_y (w);
        double height   = webkit_dom_element_get_client_height (body);

        double clientY = webkit_dom_element_get_offset_top (e);
        double clientH = webkit_dom_element_get_client_height (e);

        g_object_unref (e);

        if (height > 0) {
          if (  (clientY >= scrolled) &&
              ( (clientY + clientH) <= (scrolled + height) ))
          {
            change_focus = true;
          }
        } else {
          change_focus = true;
        }
      }

      //LOG (debug) << "focus_next_element: change: " << change_focus;

      if (change_focus) {
        focused_element++;
        apply_focus (focused_message, focused_element);
        if (!eid.empty()) scroll_to_element (eid);

        g_object_unref (body);
        g_object_unref (w);
        g_object_unref (d);
        return;
      }
    }
  }

  /* no focus change, standard behaviour */
  double scrolled = webkit_dom_dom_window_get_scroll_y (w);

  webkit_dom_dom_window_scroll_by (w, 0, STEP);

  if (force_change  || (scrolled == webkit_dom_dom_window_get_scroll_y (w))) {
    /* we're at the bottom, just move focus down */
    int focused_position = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; }) - state.messages().begin ();

    if ((focused_position + 1) < state.messages().size()) {
      focused_message = state.messages()[focused_position+1].mid();
      focused_element = 0;
      apply_focus (focused_message, focused_element);
      eid = "message_" + focused_message;
    }
  } else {
    update_focus_to_view ();
  }

  if (!eid.empty()) scroll_to_element (eid);

  g_object_unref (w);
  g_object_unref (d);
}

void AstroidExtension::focus_previous_element (bool force_change) {
  ustring eid;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);
  WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));

  if (!is_hidden (focused_message) || edit_mode) {
    /* if the message is expanded, check if we should move focus
     * to the previous element */

    auto s = *std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

    //LOG (debug) << "focus prev: current elemenet: " << s->current_element;
    /* are there any more elements */
    if (focused_element > 0) {
      /* check if the prev element is in full view */

      auto next_e = s.elements()[focused_element - 1];

      bool change_focus = force_change;

      if (!force_change) {
        if (next_e.type() != AstroidMessages::State_MessageState_Element_Type_Empty) {

          eid = next_e.sid ();

          WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());

          double scrolled = webkit_dom_dom_window_get_scroll_y (w);
          double height   = webkit_dom_element_get_client_height (body);
          // CHECK: height is 0 when there is no paging.

          double clientY = webkit_dom_element_get_offset_top (e);
          double clientH = webkit_dom_element_get_client_height (e);

          g_object_unref (e);

          if (height > 0) {
            if (  (clientY >= scrolled) &&
                ( (clientY + clientH) <= (scrolled + height) ))
            {
              change_focus = true;
            }
          } else {
            change_focus = true;
          }
        } else {
          change_focus = true;
        }
      }

      //LOG (debug) << "focus_prev_element: change: " << change_focus;

      if (change_focus) {
        focused_element--;
        apply_focus (focused_message, focused_element);
        if (!eid.empty()) scroll_to_element (eid);
        g_object_unref (body);
        g_object_unref (w);
        g_object_unref (d);
        return;
      }
    }
  }

  /* standard behaviour */
  double scrolled = webkit_dom_dom_window_get_scroll_y (w);
  /* double height   = webkit_dom_dom_window_get_screen_y (w); */

  webkit_dom_dom_window_scroll_by (w, 0, -STEP);

  if (force_change || (scrolled == webkit_dom_dom_window_get_scroll_y (w))) {
    /* we're at the top, move focus up */
    int focused_position = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; }) - state.messages().begin ();

    if (focused_position > 0) {
      focused_message = state.messages()[focused_position-1].mid();
      focused_element = state.messages()[focused_position-1].elements().size() - 1;
      apply_focus (focused_message, focused_element);
    }

    if (focused_element > 0) {
      eid = state.messages()[focused_position-1].elements()[focused_element].sid();
    } else {
      eid = "message_" + focused_message; // if no element selected, focus message
    }
  } else {
    /* scroll up */
    update_focus_to_view ();
  }

  if (!eid.empty()) scroll_to_element (eid);

  g_object_unref (body);
  g_object_unref (w);
  g_object_unref (d);
}

void AstroidExtension::focus_next_message () {
  if (edit_mode) return;

  int focused_position = std::find_if (
      state.messages().begin (),
      state.messages().end (),
      [&] (auto &m) { return m.mid () == focused_message; }) - state.messages().begin ();

  if (focused_position < static_cast<int>((state.messages().size ()-1))) {
    focused_message = state.messages()[focused_position + 1].mid ();
    focused_element = 0; // start at top
    apply_focus (focused_message, focused_element);
    scroll_to_element ("message_" + focused_message);
  }
}

void AstroidExtension::focus_previous_message (bool focus_top) {
  if (edit_mode) return;

  int focused_position = std::find_if (
      state.messages().begin (),
      state.messages().end (),
      [&] (auto &m) { return m.mid () == focused_message; }) - state.messages().begin ();

  if (focused_position > 0) {
    focused_message = state.messages()[focused_position - 1].mid();
    if (!focus_top && !is_hidden (focused_message)) {
      focused_element = state.messages()[focused_position - 1].elements().size()-1; // start at bottom
    } else {
      focused_element = 0; // start at top
    }
    apply_focus (focused_message, focused_element);
    scroll_to_element ("message_" + focused_message);
  }
}

void AstroidExtension::scroll_to_element (ustring eid) {
  if (eid.empty()) {
    cout << "ae: attempted to scroll to unspecified id." << endl;
    return;
  }

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());
  WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));

  double scrolled = webkit_dom_dom_window_get_scroll_y (w);
  double height   = webkit_dom_element_get_client_height (body);
  double upper    = webkit_dom_dom_window_get_outer_height (w);

  double clientY = webkit_dom_element_get_offset_top (e);
  double clientH = webkit_dom_element_get_client_height (e);

  if (height > 0) {
    /* if (scroll_when_visible) { */
    /*   if ((clientY + clientH - height) > upper) { */
    /*     /1* message is last, can't scroll past bottom *1/ */
    /*     webkit_dom_dom_window_scroll_to (w, 0, upper); */
    /*   } else { */
    /*     webkit_dom_dom_window_scroll_to (w, 0, clientY); */
    /*   } */
    /* } else { */
      /* only scroll if parts of the message are out view */
      if (clientY < scrolled) {
        /* top is above view  */
        webkit_dom_dom_window_scroll_to (w, 0, clientY);

      } else if ((clientY + clientH) > (scrolled + height)) {
        /* bottom is below view */

        // if message is of less height than page, scroll so that
        // bottom is aligned with bottom

        if (clientH < height) {
          webkit_dom_dom_window_scroll_to (w, 0, clientY + clientH - height);
        } else {
          // otherwise align top with top
          if ((clientY + clientH - height) > upper) {
            /* message is last, can't scroll past bottom */
            webkit_dom_dom_window_scroll_to (w, 0, upper);
          } else {
            webkit_dom_dom_window_scroll_to (w, 0, clientY);
          }
        }
      }
    /* } */
  }

  g_object_unref (body);
  g_object_unref (e);
  g_object_unref (w);
  g_object_unref (d);

  /* /1* the height does not seem to make any sense, but is still more */
  /*  * than empty. we need to re-do the calculation when everything */
  /*  * has been rendered and re-calculated. *1/ */
  /* if (height == 1) { */
  /*   in_scroll = true; */
  /*   scroll_arg = eid; */
  /*   _scroll_when_visible = scroll_when_visible; */
  /*   return false; */

  /* } else { */

  /*   return true; */

  /* } */
}

void AstroidExtension::handle_navigate (AstroidMessages::Navigate &n) {
  cout << "ae: navigating" << endl;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);


  if (n.type () == AstroidMessages::Navigate_Type_VisualBig) {

    if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      webkit_dom_dom_window_scroll_by (w, 0, BIG_JUMP);
    } else {
      webkit_dom_dom_window_scroll_by (w, 0, - BIG_JUMP);
    }
    update_focus_to_view ();

  } else if (n.type () == AstroidMessages::Navigate_Type_VisualElement) {

    if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      focus_next_element (false);
    } else {
      focus_previous_element (false);
    }

  } else if (n.type () == AstroidMessages::Navigate_Type_Element) {

    if (n.direction () == AstroidMessages::Navigate_Direction_Specific) {
      apply_focus (n.mid(), n.element ());
      if (n.element () == 0) {
        scroll_to_element ("message_" + n.mid());
      } else {
        scroll_to_element (ustring::compose ("%1", n.element ()));
      }

    } else if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      focus_next_element (true);
    } else {
      focus_previous_element (true);
    }

  } else if (n.type () == AstroidMessages::Navigate_Type_Message) {
    if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      focus_next_message ();
    } else {
      focus_previous_message (n.focus_top());
    }
  }

  g_object_unref (w);
  g_object_unref (d);
}

