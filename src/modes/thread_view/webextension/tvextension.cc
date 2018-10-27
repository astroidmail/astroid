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

/* boost::log */
# include <boost/log/core.hpp>
# include <boost/log/utility/setup/file.hpp>
# include <boost/log/utility/setup/console.hpp>
# include <boost/log/utility/setup/common_attributes.hpp>
# include <boost/log/sinks/text_file_backend.hpp>
# include <boost/log/sources/severity_logger.hpp>
# include <boost/log/sources/record_ostream.hpp>
# include <boost/log/utility/setup/filter_parser.hpp>
# include <boost/log/utility/setup/formatter_parser.hpp>
# include <boost/date_time/posix_time/posix_time_types.hpp>
# include <boost/log/expressions.hpp>
# include <boost/log/trivial.hpp>
# include <boost/log/support/date_time.hpp>
# include <boost/log/sinks/syslog_backend.hpp>
# define LOG(x) BOOST_LOG_TRIVIAL(x)
# define warn warning

# include "modes/thread_view/webextension/ae_protocol.hh"
# include "modes/thread_view/webextension/dom_utils.hh"
# include "utils/ustring_utils.hh"
# include "messages.pb.h"

namespace logging   = boost::log;
namespace keywords  = boost::log::keywords;
namespace expr      = boost::log::expressions;

using namespace Astroid;

extern "C" {/*{{{*/

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data )
{

  g_signal_connect (web_page, "send-request",
      G_CALLBACK (web_page_send_request),
      NULL);

  ext->page_created (extension, web_page, user_data);
}

bool web_page_send_request ( WebKitWebPage    *  web_page,
                             WebKitURIRequest *  request,
                             WebKitURIResponse * response,
                             gpointer            user_data)
{
  return ext->send_request (web_page, request, response, user_data);
}

G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (
    WebKitWebExtension *extension,
    gpointer pipes)
{

  /* IMPORTANT: We assume that there will only be one extension instance
   * per web page. That means that there can only be one page in each web view,
   * and each web view must use its own process. */

  ext = new AstroidExtension (extension, pipes);

  g_signal_connect (extension, "page-created",
      G_CALLBACK (web_page_created_callback),
      NULL);
}

}/*}}}*/

void AstroidExtension::init_console_log () {
  /* log to console */
  logging::formatter format =
                expr::stream
                    << "["
                    << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%H:%M:%S")
                    << "] [" << expr::attr <boost::log::attributes::current_thread_id::value_type>("ThreadID")
                    << "] [E] [" << logging::trivial::severity
                    << "] " << expr::smessage
            ;

  logging::add_console_log ()->set_formatter (format);
}

void AstroidExtension::init_sys_log () {
  typedef logging::sinks::synchronous_sink< logging::sinks::syslog_backend > sink_t;
  boost::shared_ptr< logging::core > core = logging::core::get();

  // Create a backend
  boost::shared_ptr< logging::sinks::syslog_backend > backend(new logging::sinks::syslog_backend(
        keywords::facility = logging::sinks::syslog::user,
        keywords::use_impl = logging::sinks::syslog::native,
        keywords::ident    = log_ident
        ));

  // Set the straightforward level translator for the "Severity" attribute of type int
  backend->set_severity_mapper(
      logging::sinks::syslog::direct_severity_mapping< int >("Severity"));

  // Wrap it into the frontend and register in the core.
  // The backend requires synchronization in the frontend.
  logging::core::get()->add_sink(boost::make_shared< sink_t >(backend));
}

AstroidExtension::AstroidExtension (
    WebKitWebExtension * e,
    gpointer gaddr)
{
  extension = e;

  Glib::init ();
  Gtk::Main::init_gtkmm_internals ();
  Gio::init ();
  logging::add_common_attributes ();

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

  refptr<Gio::UnixSocketAddress> addr = Gio::UnixSocketAddress::create (caddr,
      Gio::UNIX_SOCKET_ADDRESS_ABSTRACT);

  /* connect to socket */
  cli = Gio::SocketClient::create ();

  try {
    sock = cli->connect (addr);

    istream = sock->get_input_stream ();
    ostream = sock->get_output_stream ();

    /* setting up reader thread */
    reader_t = std::thread (&AstroidExtension::reader, this);

  } catch (Gio::Error &ex) {
    LOG (error) << "error: " << ex.what ();
  }
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
  LOG (debug) << "page created.";
}

bool AstroidExtension::send_request (
                    WebKitWebPage    *  /* web_page */,
                    WebKitURIRequest *  request,
                    WebKitURIResponse * /* response */,
                    gpointer            /* user_data */) {

  const char * curi = webkit_uri_request_get_uri (request);
  std::string uri (curi != NULL ? curi : "");

  LOG (debug) << "request: " << uri.substr (0, std::min (60, (int)uri.size ())) << "..";

  /* allow all requests before page has been sent. no user content has been
   * loaded yet and it seems that sometimes the request for the home uri
   * is handled here */

  if (!page_ready || allow_remote_resources) {
    LOG (debug) << "request: allow.";
    return false; // allow
  } else {
    if (find_if (allowed_uris.begin (), allowed_uris.end (),
          [&](std::string &a) {
            return (uri.substr (0, a.length ()) == a);
          }) != allowed_uris.end ())
    {
      LOG (debug) << "request: allow.";
      return false; // allow
    } else {
      LOG (debug) << "request: blocked.";
      return true; // stop
    }
  }
}

void AstroidExtension::ack (bool success) {
  /* prepare and send acknowledgement message */
  AstroidMessages::Ack m;
  m.set_success (success);

  /* send back focus */
  m.mutable_focus ()->set_mid (focused_message);
  m.mutable_focus ()->set_element (focused_element);
  m.mutable_focus ()->set_focus (true);

  AeProtocol::send_message_async (AeProtocol::MessageTypes::Ack, m, ostream, m_ostream);
}

void AstroidExtension::reader () {/*{{{*/
  LOG (debug) << "reader thread: started.";

  while (run) {
    LOG (debug) << "reader waiting..";

    std::vector<gchar> buffer;
    AeProtocol::MessageTypes mt;

    try {

      mt = AeProtocol::read_message (
          istream,
          reader_cancel,
          buffer);

    } catch (AeProtocol::ipc_error &e) {
      LOG (warn) << "reader thread: " << e.what ();
      run = false;
      break;
    }

    /* parse message */
    switch (mt) {
      case AeProtocol::MessageTypes::Debug:
        {
          AstroidMessages::Debug m;
          m.ParseFromArray (buffer.data(), buffer.size());
          LOG (debug) << m.msg ();
          ack (true);
        }
        break;

      case AeProtocol::MessageTypes::Mark:
        {
          AstroidMessages::Mark m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_mark), m));
        }
        break;

      case AeProtocol::MessageTypes::Hidden:
        {
          AstroidMessages::Hidden m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              [this,m] () {
                set_hidden (m.mid (), m.hidden ());
                ack (true);
              });
        }
        break;

      case AeProtocol::MessageTypes::Focus:
        {
          AstroidMessages::Focus m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_focus), m));
        }
        break;

      case AeProtocol::MessageTypes::State:
        {
          AstroidMessages::State m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_state), m));
        }
        break;

      case AeProtocol::MessageTypes::Indent:
        {
          AstroidMessages::Indent m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              [this,m] () {
                set_indent (m.indent ());
                ack (true);
              });
        }
        break;

      case AeProtocol::MessageTypes::AllowRemoteImages:
        {
          AstroidMessages::AllowRemoteImages m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              [this,m] () {
                allow_remote_resources = true;
                reload_images ();
                ack (true);
              });
        }
        break;

      case AeProtocol::MessageTypes::Page:
        {
          AstroidMessages::Page s;
          s.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_page), s));
        }
        break;

      case AeProtocol::MessageTypes::ClearMessages:
        {
          AstroidMessages::ClearMessage m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::clear_messages), m));
        }
        break;

      case AeProtocol::MessageTypes::AddMessage:
        {
          AstroidMessages::Message m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::add_message), m));
        }
        break;

      case AeProtocol::MessageTypes::UpdateMessage:
        {
          AstroidMessages::UpdateMessage m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::update_message), m));
        }
        break;

      case AeProtocol::MessageTypes::RemoveMessage:
        {
          AstroidMessages::Message m;
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::remove_message), m));
        }
        break;

      case AeProtocol::MessageTypes::Info:
        {
          AstroidMessages::Info m;
          m.ParseFromArray (buffer.data(), buffer.size());

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
          m.ParseFromArray (buffer.data(), buffer.size());
          Glib::signal_idle().connect_once (
              sigc::bind (
                sigc::mem_fun(*this, &AstroidExtension::handle_navigate), m));
        }
        break;

      default:
        break; // unknown message
    }
  }

  LOG (debug) << "reader thread exit.";
}/*}}}*/

void AstroidExtension::handle_page (AstroidMessages::Page &s) {/*{{{*/
  /* set up logging */
  if (s.use_stdout ()) {
    init_console_log ();
  }

  if (s.use_syslog ()) {
    init_sys_log ();
  }

  if (s.disable_log ()) {
    logging::core::get()->set_logging_enabled (false);
  }

  logging::core::get()->set_filter (logging::trivial::severity >= sevmap[s.log_level ()]);

  GError *err = NULL;
  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);

  /* load html */
  LOG (debug) << "loading html..";

  WebKitDOMElement * he = webkit_dom_document_create_element (d, "HTML", (err = NULL, &err));
  webkit_dom_element_set_outer_html (he, s.html ().c_str (), (err = NULL, &err));

  webkit_dom_document_set_body (d, WEBKIT_DOM_HTML_ELEMENT(he), (err = NULL, &err));

  /* load css style */
  LOG (debug) << "loading stylesheet..";
  WebKitDOMElement  *e = webkit_dom_document_create_element (d, "STYLE", (err = NULL, &err));

  WebKitDOMText *t = webkit_dom_document_create_text_node
    (d, s.css().c_str());

  webkit_dom_node_append_child (WEBKIT_DOM_NODE(e), WEBKIT_DOM_NODE(t), (err = NULL, &err));

  WebKitDOMHTMLHeadElement * head = webkit_dom_document_get_head (d);
  webkit_dom_node_append_child (WEBKIT_DOM_NODE(head), WEBKIT_DOM_NODE(e), (err = NULL, &err));
  LOG (debug) << "done";

  /* store part / iframe css for later */
  part_css = s.part_css ();

  /* store allowed uris */
  for (auto &s : s.allowed_uris ()) {
    allowed_uris.push_back (s);
  }

  page_ready = true;

  g_object_unref (he);
  g_object_unref (head);
  g_object_unref (t);
  g_object_unref (e);
  g_object_unref (d);

  ack (true);
}/*}}}*/

void AstroidExtension::reload_images () {
  LOG (debug) << "reload images.";
  GError * err = NULL;
  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);

  for (auto &m : state.messages()) {
    ustring div_id = "message_" + m.mid ();
    WebKitDOMElement * me = webkit_dom_document_get_element_by_id (d, div_id.c_str());

    for (auto &c : m.elements()) {
      if (!c.focusable ()) {
        WebKitDOMHTMLElement * body_container = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_element_by_id (d, c.sid ().c_str ()));
        WebKitDOMHTMLElement * iframe = DomUtils::select (WEBKIT_DOM_NODE(body_container), ".body_iframe");
        WebKitDOMDocument * iframe_d = webkit_dom_html_iframe_element_get_content_document (WEBKIT_DOM_HTML_IFRAME_ELEMENT(iframe));
        WebKitDOMHTMLElement * b = webkit_dom_document_get_body (iframe_d);

        WebKitDOMNodeList * imgs = webkit_dom_element_query_selector_all (WEBKIT_DOM_ELEMENT(b), "img", (err = NULL, &err));

        gulong l = webkit_dom_node_list_get_length (imgs);
        for (gulong i = 0; i < l; i++) {

          WebKitDOMNode * in = webkit_dom_node_list_item (imgs, i);
          WebKitDOMElement * ine = WEBKIT_DOM_ELEMENT (in);

          if (ine != NULL) {
            gchar * src = webkit_dom_element_get_attribute (ine, "src");
            if (src != NULL) {
              ustring usrc (src);
              /* replace CID images with real image */
              if (usrc.substr (0, 4) == "cid:") {
                ustring cid = usrc.substr (4, std::string::npos);
                LOG (debug) << "CID: " << cid;

                auto s = std::find_if ( messages[m.mid()].attachments().begin (),
                                        messages[m.mid()].attachments().end (),
                                        [&] (auto &a) { return a.cid() == cid; } );

                if (s != messages[m.mid()].attachments().end ()) {
                  LOG (debug) << "found matching attachment for CID.";

                  webkit_dom_element_set_attribute (ine, "src", "", (err = NULL, &err));
                  webkit_dom_element_set_attribute (ine, "src", s->content().c_str (), (err = NULL, &err));

                } else {
                  LOG (warn) << "could not find matching attachment for CID.";
                }
              } else {

                /* trigger reload */
                webkit_dom_element_set_attribute (ine, "src", "", (err = NULL, &err));
                webkit_dom_element_set_attribute (ine, "src", src, (err = NULL, &err));
              }
            }
          }

          g_object_unref (in);
        }

        g_object_unref (imgs);
        g_object_unref (b);
        g_object_unref (iframe_d);
        g_object_unref (iframe);
        g_object_unref (body_container);
      }
    }
    g_object_unref (me);
  }

  g_object_unref (d);
}

void AstroidExtension::handle_state (AstroidMessages::State &s) {/*{{{*/
  LOG (debug) << "got state.";
  state = s;
  edit_mode = state.edit_mode ();
  ack (true);
}/*}}}*/

void AstroidExtension::set_indent (bool indent) {
  LOG (debug) << "update indent.";
  indent_messages = indent;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);

  for (auto &m : state.messages()) {
    ustring mid = "message_" + m.mid ();

    GError * err = NULL;

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    /* set indentation based on level */
    if (indent_messages && m.level() > 0) {
      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (e),
          "style", ustring::compose ("margin-left: %1px", int(m.level() * INDENT_PX)).c_str(), (err = NULL, &err));
    } else {
      webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (e), "style");
    }

    g_object_unref (e);
  }

  g_object_unref (d);
}

void AstroidExtension::clear_messages (AstroidMessages::ClearMessage &) {
  LOG (debug) << "clearing all messages.";

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * container = DomUtils::get_by_id (d, "message_container");

  GError *err = NULL;

  webkit_dom_element_set_inner_html (container, "<span id=\"placeholder\"></span>", (err = NULL, &err));

  g_object_unref (container);
  g_object_unref (d);

  /* reset */
  focused_message = "";
  focused_element = -1;
  messages.clear ();
  state = AstroidMessages::State();
  allow_remote_resources = false;
  indent_messages = false;

  ack (true);
}

// Message generation {{{
void AstroidExtension::add_message (AstroidMessages::Message &m) {
  LOG (debug) << "adding message: " << m.mid ();
  messages[m.mid()] = m;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
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

  LOG (debug) << "message added.";

  apply_focus (focused_message, focused_element); // in case we got focus before message was added.

  ack (true);
}

void AstroidExtension::remove_message (AstroidMessages::Message &m) {
  LOG (debug) << "removing message: " << m.mid ();
  messages.erase (m.mid());

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * container = DomUtils::get_by_id (d, "message_container");

  ustring div_id = "message_" + m.mid();
  WebKitDOMHTMLElement * div_message = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_element_by_id (d, div_id.c_str()));

  GError * err = NULL;
  webkit_dom_node_remove_child (WEBKIT_DOM_NODE(container), WEBKIT_DOM_NODE (div_message), (err = NULL, &err));

  g_object_unref (div_message);
  g_object_unref (container);
  g_object_unref (d);

  LOG (debug) << "message removed.";

  ack (true);
}

void AstroidExtension::update_message (AstroidMessages::UpdateMessage &um) {
  auto m = um.m();
  messages[m.mid()] = m;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * container = DomUtils::get_by_id (d, "message_container");

  ustring div_id = "message_" + m.mid();

  WebKitDOMHTMLElement * old_div_message = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_element_by_id (d, div_id.c_str()));

  if (um.type () == AstroidMessages::UpdateMessage_Type_VisibleParts) {
    LOG (debug) << "updating message: " << m.mid () << " (full update)";
    /* various states */
    bool hidden = is_hidden (m.mid ());
    // TODO: info and warning

    GError * err = NULL;

    WebKitDOMHTMLElement * div_message = DomUtils::make_message_div (d);
    webkit_dom_element_set_id (WEBKIT_DOM_ELEMENT (div_message), div_id.c_str());
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

    webkit_dom_node_replace_child (WEBKIT_DOM_NODE(container), WEBKIT_DOM_NODE (div_message), WEBKIT_DOM_NODE (old_div_message), (err = NULL, &err));

    /* set various state */
    set_hidden (m.mid (), hidden);
    set_indent (indent_messages);

    g_object_unref (div_message);

    auto ms = std::find_if (
        state.messages().begin(),
        state.messages().end(),
        [&] (auto m) {
          return m.mid() == focused_message;
        });

    if (!ms->elements(focused_element).focusable()) {
      /* find next or previous element */

      /* are there any more focusable elements */
      auto next_e = std::find_if (
          ms->elements().begin () + (focused_element +1),
          ms->elements().end (),
          [&] (auto &e) { return e.focusable (); });

      if (next_e != ms->elements().end()) {
        focused_element = std::distance (ms->elements ().begin (), next_e);

      } else {
        LOG (debug) << "take previous";
        /* take previous element */
        auto next_e = std::find_if (
            ms->elements().rbegin() +
              (ms->elements().size() - focused_element),

            ms->elements().rend (),
            [&] (auto &e) { return e.focusable (); });

        if (next_e != ms->elements().rend ()) {
          /* previous */
          focused_element = std::distance (ms->elements ().begin (), next_e.base() -1);
        } else {
          /* message */
          focused_element = 0;
        }
      }

    }

    apply_focus (focused_message, focused_element);

  } else if (um.type () == AstroidMessages::UpdateMessage_Type_Tags) {
    LOG (debug) << "updating message: " << m.mid () << " (tags only)";
    message_render_tags (m, WEBKIT_DOM_HTML_ELEMENT(old_div_message));
    message_update_css_tags (m, WEBKIT_DOM_HTML_ELEMENT(old_div_message));
  }

  g_object_unref (old_div_message);
  g_object_unref (container);
  g_object_unref (d);

  ack (true);
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

  header += create_header_row ("Tags", "", false, false, true);

  webkit_dom_element_set_inner_html (
      WEBKIT_DOM_ELEMENT(table_header),
      header.c_str(),
      (err = NULL, &err));

  if (m.tags().size () > 0) {
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

   LOG (debug) << "create message part: " << c.id() << " (siblings: " << c.sibling() << ") (kids: " << c.kids().size() << ")" <<
    " (attachment: " << c.attachment() << ")" << " (viewable: " << c.viewable() << ")" << " (focusable: " << c.focusable () << ")" << " (mimetype: " << mime_type << ")";

  if (c.use()) {
    if (!c.focusable () && c.viewable()) {
      create_body_part (message, c, span_body);
    } else if (c.viewable()) {
      create_sibling_part (c, span_body);
    }

    /* descend */
    for (auto &k: c.kids()) {
      create_message_part_html (message, k, span_body);
    }
  } else {
    if (!c.focusable ()) {
      create_body_part (message, c, span_body);
    } else {
      create_sibling_part (c, span_body);
    }
  }
}

void AstroidExtension::create_body_part (
    const AstroidMessages::Message &message,
    const AstroidMessages::Message::Chunk &c,
    WebKitDOMHTMLElement * span_body)
{
  // <span id="body_template" class="body_part"></span>

  LOG (debug) << "create body part: " << c.id();

  GError *err;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMHTMLElement * body_container =
    DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#body_template");

  webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (body_container),
      "id");

  webkit_dom_element_set_id (WEBKIT_DOM_ELEMENT (body_container),
      c.sid().c_str ());

  ustring body = c.content();

  /* check encryption */
  //
  //  <div id="encrypt_template" class=encrypt_container">
  //      <div class="message"></div>
  //  </div>
  if (c.is_encrypted() || c.is_signed()) {
    WebKitDOMHTMLElement * encrypt_container =
      DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#encrypt_template");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (encrypt_container),
        "id");

    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (encrypt_container),
      "id", ustring::compose ("%1", c.crypto_id()).c_str (),
      (err = NULL, &err));

    WebKitDOMDOMTokenList * class_list_e =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(encrypt_container));


    ustring content = "";

    ustring sign_string = "";
    ustring enc_string  = "";

    vector<ustring> all_sig_errors;

    if (c.is_signed ()) {

      if (c.signature().verified()) {
        sign_string += "<span class=\"header\">Signature verification succeeded.</span>";
      } else {
        sign_string += "<span class=\"header\">Signature verification failed!</span>";
      }

      for (auto &s : c.signature().sign_strings ()) {
        sign_string += s;
      }
    }

    if (c.is_encrypted ()) {
      if (c.is_signed ()) enc_string = "<span class=\"header\">Signed and Encrypted.</span>";
      else                enc_string = "<span class=\"header\">Encrypted.</span>";


      if (c.encryption().decrypted ()) {
        for (auto &e : c.encryption ().enc_strings ()) {
          enc_string += e;
        }

        if (c.is_signed ()) enc_string += "<br /><br />";

      } else {
        enc_string += "Encryption: Failed decryption.";

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

    if (c.is_encrypted ()) {
      DomUtils::switch_class (class_list_e, "encrypted", true);
      DomUtils::switch_class (class_list, "encrypted", true);

      if (!c.encryption().decrypted()) {
        DomUtils::switch_class (class_list_e, "decrypt_failed", true);
        DomUtils::switch_class (class_list, "decrypt_failed", true);
      }
    }

    if (c.is_signed ()) {
      DomUtils::switch_class (class_list_e, "signed", true);
      DomUtils::switch_class (class_list, "signed", true);

      if (!c.signature().verified()) {
        DomUtils::switch_class (class_list_e, "verify_failed", true);
        DomUtils::switch_class (class_list, "verify_failed", true);

        /* add specific errors */
        std::sort (all_sig_errors.begin (), all_sig_errors.end ());
        all_sig_errors.erase (unique (all_sig_errors.begin (), all_sig_errors.end ()), all_sig_errors.end ());

        for (ustring & e : all_sig_errors) {
          DomUtils::switch_class (class_list_e, e, true);
          DomUtils::switch_class (class_list, e, true);
        }
      }
    }

    g_object_unref (class_list);
    g_object_unref (class_list_e);
    g_object_unref (encrypt_container);
    g_object_unref (message_cont);

  }

  webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
      WEBKIT_DOM_NODE (body_container), (err = NULL, &err));

  g_object_unref (d);

  /*
   * run this later on extension GUI thread in order to make sure that the "body part" has been
   * added to the document.
   */
  Glib::signal_idle().connect_once (
      sigc::bind (
        sigc::mem_fun(*this, &AstroidExtension::set_iframe_src), message.mid(), c.sid(), body));

  LOG (debug) << "create_body_part done.";
}

void AstroidExtension::set_iframe_src (ustring mid, ustring cid, ustring body) {
  LOG (debug) << "set iframe src: " << mid << ", " << cid;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  GError *err;

  WebKitDOMHTMLElement * body_container = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_element_by_id (d, cid.c_str ()));

  WebKitDOMHTMLElement * iframe =
    DomUtils::select (WEBKIT_DOM_NODE(body_container), ".body_iframe");


  /* by using srcdoc we avoid creating any requests that would have to be
   * allowed on the main GUI thread. even if we run this function async there
   * might be other sync calls to the webextension that cause blocking since
   * most webextension funcs need to run on extension GUI thread in order to
   * manipulate DOM tree */

  /* according to: http://w3c.github.io/html/semantics-embedded-content.html#element-attrdef-iframe-src
   * we need to escape quotation marks and amperands. it seems that by using
   * this call webkit does this for us. this is critical since otherwise the
   * content could break out of the iframe. */

  /* it would probably be possible to mess up the style, but it should only affect the current frame content. this would anyway be possible. */

  webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (iframe), "srcdoc",
      ustring::compose (
        "<STYLE>%1</STYLE>%2",
        part_css,
        body ).c_str (),
      (err = NULL, &err));

  g_object_unref (iframe);
  g_object_unref (body_container);
  g_object_unref (d);
}

void AstroidExtension::create_sibling_part (
    /* const AstroidMessages::Message &message, */
    const AstroidMessages::Message::Chunk &sibling,
    WebKitDOMHTMLElement * span_body) {

  LOG (debug) << "create sibling part: " << sibling.id ();
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

  ustring content = ustring::compose ("Alternative part (type: %1)%2",
      Glib::Markup::escape_text(sibling.mime_type ()),
      (sibling.mime_type() != "text/plain" ? " - potentially sketchy." : ""));

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
    LOG (debug) << "create mime message part: " << c.id();
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


    if (c.is_signed () || c.is_encrypted ()) {
      /* add encryption or signed tag to attachment */
      WebKitDOMDOMTokenList * class_list =
        webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(attachment_table));

      if (c.is_encrypted ()) {
        DomUtils::switch_class (class_list, "encrypted", true);
      }

      if (c.is_signed ()) {
        DomUtils::switch_class (class_list, "signed", true);
      }

      g_object_unref (class_list);
    }

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
      DomUtils::assemble_data_uri (image_content_type.c_str (), content, content_size).c_str(), &err);

  g_object_unref (attachment_icon_img);

  attachment_icon_img = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".attachment.icon.sec");
  img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (attachment_icon_img);

  err = NULL;
  webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
      DomUtils::assemble_data_uri (image_content_type.c_str (), content, content_size).c_str(), &err);

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

  webkit_dom_html_image_element_set_src (img, DomUtils::assemble_data_uri (image_content_type.c_str (), content, content_size).c_str());

  g_object_unref (marked_icon_img);

  marked_icon_img = DomUtils::select (
      WEBKIT_DOM_NODE (div_message),
      ".marked.icon.sec");
  img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (marked_icon_img);

  webkit_dom_html_image_element_set_src (img, DomUtils::assemble_data_uri (image_content_type.c_str (), content, content_size).c_str());

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
    hide_warning (m); // ack's
    return;
  }
  LOG (debug) << "set warning: " << m.txt ();

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
  ack (true);
}

void AstroidExtension::hide_warning (AstroidMessages::Info &m)
{
  LOG (debug) << "hide warning.";
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

  ack (true);
}

void AstroidExtension::set_info (AstroidMessages::Info &m)
{
  if (!m.set ()) {
    hide_info (m); // ack's
    return;
  }
  LOG (debug) << "set info: " << m.txt ();

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

  ack (true);
}

void AstroidExtension::hide_info (AstroidMessages::Info &m) {
  LOG (debug) << "hide info.";
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

  ack (true);
}

/* }}} */

void AstroidExtension::set_hidden (ustring mid, bool hidden) {
  /* hide or show message */
  LOG (debug) << "set hidden";
  ustring div_id = "message_" + mid;

  GError * err = NULL;

  WebKitDOMDocument *d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, div_id.c_str());

  WebKitDOMDOMTokenList * class_list =
    webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

  if (hidden) {
    LOG (debug) << "hide: " << mid;
    webkit_dom_dom_token_list_toggle (class_list, "hide", hidden, &err );
  } else if (webkit_dom_dom_token_list_contains (class_list, "hide")) {
    LOG (debug) << "show: " << mid;
    webkit_dom_dom_token_list_toggle (class_list, "hide", false, &err );
  }

  /* if the message we just hid or showed is not the focused one it may have
   * caused the focused message to go out of view. scroll to original focused
   * message in that case. */

  if (mid != focused_message && !focused_message.empty()) {
    if (focused_element > 0) {
      scroll_to_element (
          std::find_if (state.messages().begin (), state.messages().end (),
            [&] (auto &m) { return m.mid () == focused_message; })->elements(focused_element).sid ()
          );
    } else {
      ustring div = "message_" + focused_message;
      scroll_to_element (div);
    }
  }

  g_object_unref (class_list);
  g_object_unref (e);
  g_object_unref (d);
}

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

  ack (true);
}/*}}}*/

void AstroidExtension::handle_focus (AstroidMessages::Focus &msg) {
  apply_focus (msg.mid (), msg.element ());
  ack (true);
}

void AstroidExtension::apply_focus (ustring mid, int element) {
  LOG (debug) << "focusing: " << mid << ": " << element;
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

    int ei = 0;
    for (auto &e : m.elements ()) {
      if (
          // all states contain an empty element at first
          e.type() != AstroidMessages::State_MessageState_Element_Type_Empty

          // skip elements that cannot be focused
          && e.focusable() == true
         ) {

        WebKitDOMElement * ee = webkit_dom_document_get_element_by_id (d, e.sid().c_str());
        WebKitDOMDOMTokenList * e_class_list =
          webkit_dom_element_get_class_list (ee);

        DomUtils::switch_class (e_class_list, "focused", (m.mid () == mid && ei == element));

        g_object_unref (e_class_list);
        g_object_unref (ee);
      }

      ei++;
    }

    g_object_unref (me);
  }

  g_object_unref (d);

  LOG (debug) << "focus done.";
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
    focused_message = state.messages(0).mid() ;
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
  LOG (debug) << "fne: " << focused_message << ", " << focused_element;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);
  WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));

  /* force change if we are at maximum scroll */
  long scroll_height = webkit_dom_element_get_scroll_height (body);
  long scroll_top    = webkit_dom_element_get_scroll_top (body);
  long body_height   = webkit_dom_element_get_client_height (body);

  force_change = force_change ||
    (scroll_height - scroll_top == body_height);


  if (!is_hidden (focused_message)) {
    /* if the message is expanded, check if we should move focus
     * to the next element */

    auto s = *std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

    /* are there any more focusable elements */
    auto next_e = std::find_if (
        s.elements().begin () + (focused_element +1),
        s.elements().end (),
        [&] (auto &e) { return e.focusable (); });

    if (next_e != s.elements().end ()) {
      /* check if the next element is in full view */
      eid = next_e->sid ();

      LOG (debug) << "next_e: " << eid;

      if (force_change || DomUtils::in_view (page, eid)) {
        /* move focus to next element and scroll if necessary */
        focused_element = std::distance (s.elements ().begin (), next_e);
        apply_focus (focused_message, focused_element);

        scroll_to_element (eid);
        g_object_unref (body);
        g_object_unref (w);
        g_object_unref (d);
        return;
      }

      /* fall through to scroll */
    } else {
      /* move focus to next message if in view or force_change */
      auto s = std::find_if (
          state.messages().begin (),
          state.messages().end (),
          [&] (auto &m) { return m.mid () == focused_message; });

      s++;

      if (s < state.messages().end () &&
          (force_change || DomUtils::in_view (page, "message_" + s->mid ()))) {

        focus_next_message ();

        g_object_unref (body);
        g_object_unref (w);
        g_object_unref (d);
        return;
      }
      /* fall through to scroll */
    }
  } else {
    /* move focus to next message if in view or force_change */

    auto s = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

    s++;

    if (s < state.messages().end () &&
        (force_change || DomUtils::in_view (page, "message_" + s->mid ()))) {

      focus_next_message ();

      g_object_unref (body);
      g_object_unref (w);
      g_object_unref (d);
      return;
    }
    /* fall through to scroll */
  }

  /* no focus change, standard behaviour */

  webkit_dom_dom_window_scroll_by (w, 0, STEP);

  if (!force_change) {
    update_focus_to_view ();
  }

  g_object_unref (body);
  g_object_unref (w);
  g_object_unref (d);
}

void AstroidExtension::focus_previous_element (bool force_change) {
  ustring eid;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);
  WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));

  /* force change if scrolled to top */
  long scroll_top = webkit_dom_element_get_scroll_top (body);
  force_change    = force_change || (scroll_top == 0);



  if (!is_hidden (focused_message)) {
    /* if the message is expanded, check if we should move focus
     * to the previous element */

    LOG (debug) << "fpe: prev, not hidden";

    auto s = *std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

    /* focus previous focusable element */
    auto next_e = std::find_if (
        s.elements().rbegin() + (s.elements().size() - focused_element),
        s.elements().rend (),
        [&] (auto &e) { return e.focusable (); });


    if (next_e != s.elements().rend()) {
      /* check if the prev element is in full view */
      eid = next_e->sid ();
      LOG (debug) << "fpe: more prev items: " << eid;

      if (force_change || DomUtils::in_view (page, eid)) {

        focused_element = std::distance (s.elements ().begin (), next_e.base() -1);

        apply_focus (focused_message, focused_element);

        if (next_e->id() != -1) {
          /* if part element (always id == -1), don't scroll to empty part */
          scroll_to_element (eid);
        }

        g_object_unref (body);
        g_object_unref (w);
        g_object_unref (d);
        return;
      }

      /* fall through to scroll */

    }  else {
      /* focus previous message if in view */
      auto s = std::find_if (
          state.messages().begin (),
          state.messages().end (),
          [&] (auto &m) { return m.mid () == focused_message; });

      s--;

      if (s >= state.messages().begin () && (
            force_change || DomUtils::in_view (page, "message_" + s->mid ()) )) {

        focus_previous_message (false);

        g_object_unref (body);
        g_object_unref (w);
        g_object_unref (d);
        return;
      }

      /* fall through to scroll */
    }
  } else {
    /* move focus to previous message if in view or force_change */
    auto s = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

    s--;

    if (s >= state.messages().begin () && (
          force_change || DomUtils::in_view (page, "message_" + s->mid ()) )) {

      focus_previous_message (false);

      g_object_unref (body);
      g_object_unref (w);
      g_object_unref (d);
      return;
    }
    /* fall through to scroll */
  }

  /* standard behaviour */
  webkit_dom_dom_window_scroll_by (w, 0, -STEP);

  if (!force_change) {
    /* we have scrolled */
    update_focus_to_view ();
  }

  g_object_unref (body);
  g_object_unref (w);
  g_object_unref (d);
}

void AstroidExtension::focus_next_message () {
  if (edit_mode) return;

  auto s = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

  s++;

  if (s < state.messages().end ()) {
    focused_message = s->mid ();
    focused_element = 0; // start at top
    apply_focus (focused_message, focused_element);
    scroll_to_element ("message_" + focused_message);
  }
}

void AstroidExtension::focus_previous_message (bool focus_top) {
  if (edit_mode) return;
  LOG (debug) << "prev message";

  auto s = std::find_if (
        state.messages().begin (),
        state.messages().end (),
        [&] (auto &m) { return m.mid () == focused_message; });

  s--;

  if (s >= state.messages().begin ()) {
    focused_message = s->mid();
    if (!focus_top && !is_hidden (focused_message)) {
      /* focus last focusable element */
      auto next_e = std::find_if (
          s->elements().rbegin(),
          s->elements().rend (),
          [&] (auto &e) { return e.focusable (); });

      if (next_e != s->elements ().rend ()) {

        focused_element = std::distance (s->elements ().begin (), next_e.base() -1);

      } else {
        focused_element = 0;
      }
    } else {
      focused_element = 0;
    }

    apply_focus (focused_message, focused_element);
    scroll_to_element ("message_" + focused_message);
  }
}

void AstroidExtension::scroll_to_element (ustring eid) {
  LOG (debug) << "scrolling to: " << eid;
  if (eid.empty()) {
    LOG (debug) << "attempted to scroll to unspecified id.";
    return;
  }

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());

  webkit_dom_element_scroll_into_view_if_needed (e, false);

  g_object_unref (e);
  g_object_unref (d);
  return;
}

void AstroidExtension::handle_navigate (AstroidMessages::Navigate &n) {
  std::string _t = AstroidMessages::Navigate_Type_descriptor ()->FindValueByNumber (n.type ())->name ();
  LOG (debug) << "navigating, type: " << _t;

  WebKitDOMDocument * d = webkit_web_page_get_dom_document (page);
  WebKitDOMDOMWindow * w = webkit_dom_document_get_default_view (d);

  if (n.type () == AstroidMessages::Navigate_Type_VisualBig) {

    if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      webkit_dom_dom_window_scroll_by (w, 0, BIG_JUMP);
    } else {
      webkit_dom_dom_window_scroll_by (w, 0, -BIG_JUMP);
    }
    update_focus_to_view ();

  } else if (n.type () == AstroidMessages::Navigate_Type_VisualPage) {

    if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      webkit_dom_dom_window_scroll_by (w, 0, PAGE_JUMP);
    } else {
      webkit_dom_dom_window_scroll_by (w, 0, -PAGE_JUMP);
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

  } else if (n.type () == AstroidMessages::Navigate_Type_FocusView) {
    update_focus_to_view ();

  } else if (n.type () == AstroidMessages::Navigate_Type_Extreme) {
    if (n.direction () == AstroidMessages::Navigate_Direction_Down) {
      WebKitDOMElement * body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body (d));
      double scroll_height = webkit_dom_element_get_scroll_height (body);

      webkit_dom_dom_window_scroll_to (w, 0, scroll_height);

      /* focus last element of last message */
      auto s = --state.messages ().end ();
      focused_message = s->mid ();

      if (is_hidden (focused_message)) {
        focused_element = 0;
      } else {
        auto next_e = std::find_if (
            s->elements().rbegin(),
            s->elements().rend (),
            [&] (auto &e) { return e.focusable (); });

        if (next_e != s->elements ().rend ()) {

          focused_element = std::distance (s->elements ().begin (), next_e.base() -1);

        } else {
          focused_element = 0;
        }
      }

      apply_focus (focused_message, focused_element);

      g_object_unref (body);
    } else {
      webkit_dom_dom_window_scroll_to (w, 0, 0);
      apply_focus (state.messages().begin()->mid (), 0);
    }
  }

  LOG (debug) << "navigation done.";

  g_object_unref (w);
  g_object_unref (d);

  ack (true);
}

