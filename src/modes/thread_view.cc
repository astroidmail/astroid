# include <iostream>
# include <fstream>
# include <atomic>
# include <vector>
# include <algorithm>

# include <gtkmm.h>
# include <webkit/webkit.h>
# include <gio/gio.h>

# include "thread_view.hh"
# include "main_window.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "db.hh"
# include "utils/utils.hh"
# include "utils/mail_quote.hh"
# include "utils/address.hh"
# include "utils/vector_utils.hh"
# include "actions/action.hh"
# include "actions/tag_action.hh"
# include "reply_message.hh"
# include "forward_message.hh"
# include "raw_message.hh"
# include "log.hh"


using namespace std;

namespace Astroid {
  bool ThreadView::theme_loaded = false;
  const char * ThreadView::thread_view_html_f = "ui/thread-view.html";
  const char * ThreadView::thread_view_css_f  = "ui/thread-view.css";
  ustring ThreadView::thread_view_html;
  ustring ThreadView::thread_view_css;

  ThreadView::ThreadView (MainWindow * mw) {
    main_window = mw;
    tab_widget = Gtk::manage(new Gtk::Label (""));

    pack_start (scroll, true, true, 5);

    /* set up webkit web view (using C api) */
    webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    /* dpi */
    /*
    WebKitViewportAttributes* attributes = webkit_web_view_get_viewport_attributes (webview);

    gint dpi;
    gfloat dpr;
    g_object_get (G_OBJECT (attributes), "device-dpi", &dpi, NULL);
    g_object_get (G_OBJECT (attributes), "device-pixel-ratio", &dpr, NULL);

    log << debug << "web: dpi: " << dpi << ", dpr: " << dpr << endl;
    auto win = get_screen ();
    double gdpi = win->get_resolution ();

    log << debug << "gdk: dpi: " << gdpi << endl;

    g_object_set (G_OBJECT (attributes), "device-dpi", (int)gdpi, NULL);

    webkit_viewport_attributes_recompute (attributes);

    g_object_get (G_OBJECT (attributes), "device-dpi", &dpi, NULL);
    g_object_get (G_OBJECT (attributes), "device-pixel-ratio", &dpr, NULL);

    log << debug << "web: dpi: " << dpi << ", dpr: " << dpr << endl;

    bool valid;
    g_object_get (G_OBJECT (attributes), "valid", &valid, NULL);
    log << debug << "web: valid: " << valid << endl;
    */

    websettings = WEBKIT_WEB_SETTINGS (webkit_web_settings_new ());
    g_object_set (G_OBJECT(websettings),
        "enable-scripts", FALSE,
        "enable-java-applet", FALSE,
        "enable-plugins", FALSE,
        "auto-load-images", TRUE,
        "enable-display-of-insecure-content", FALSE,
        "enable-dns-prefetching", FALSE,
        "enable-fullscreen", FALSE,
        "enable-html5-database", FALSE,
        "enable-html5-local-storage", FALSE,
     /* "enable-mediastream", FALSE, */
        "enable-mediasource", FALSE,
        "enable-offline-web-application-cache", FALSE,
        "enable-page-cache", FALSE,
        "enable-private-browsing", TRUE,
        "enable-running-of-insecure-content", FALSE,
        "enable-display-of-insecure-content", FALSE,
        "enable-xss-auditor", TRUE,
        "media-playback-requires-user-gesture", TRUE,
        "enable-developer-extras", TRUE, // TODO: should only enabled conditionally

        NULL);

    webkit_web_view_set_settings (webview, websettings);


    gtk_container_add (GTK_CONTAINER (scroll.gobj()), GTK_WIDGET(webview));

    scroll.show_all ();

    /* load css, html and DOM objects */
    if (!theme_loaded) {
      ifstream tv_html_f (thread_view_html_f);
      istreambuf_iterator<char> eos; // default is eos
      istreambuf_iterator<char> tv_iit (tv_html_f);

      thread_view_html.append (tv_iit, eos);
      tv_html_f.close ();

      ifstream tv_css_f (thread_view_css_f);
      istreambuf_iterator<char> tv_css_iit (tv_css_f);
      thread_view_css.append (tv_css_iit, eos);
      tv_css_f.close ();

      theme_loaded = true;
    }

    wk_loaded = false;

    g_signal_connect (webview, "notify::load-status",
        G_CALLBACK(ThreadView_on_load_changed),
        (gpointer) this );


    add_events (Gdk::KEY_PRESS_MASK);

    /* set up web view inspector */
    web_inspector = webkit_web_view_get_inspector (webview);
    g_signal_connect (web_inspector, "inspect-web-view",
        G_CALLBACK(ThreadView_activate_inspector),
        (gpointer) this);

    g_signal_connect (web_inspector, "show-window",
        G_CALLBACK(ThreadView_show_inspector),
        (gpointer) this);
  }

  ThreadView::~ThreadView () {
    log << debug << "tv: deconstruct." << endl;
    // TODO: possibly still some errors here in paned mode
    //g_object_unref (webview); // probably garbage collected since it has a parent widget
    g_object_unref (websettings);
    if (container) g_object_unref (container);
  }

  /* is this callback setup safe?
   *
   * http://stackoverflow.com/questions/2068022/in-c-is-it-safe-portable-to-use-static-member-function-pointer-for-c-api-call
   *
   * http://gtk.10911.n7.nabble.com/Using-g-signal-connect-in-class-td57137.html
   *
   * to be portable we have to use a free function declared extern "C". a
   * static member function is likely to work at least on gcc/g++, but not
   * necessarily elsewhere.
   *
   */

  extern "C" bool ThreadView_on_load_changed (
      GtkWidget *       w,
      GParamSpec *      p,
      gpointer          data )
  {
    return ((ThreadView *) data)->on_load_changed (w, p);
  }

  bool ThreadView::on_load_changed (
      GtkWidget *       w,
      GParamSpec *      p)
  {
    WebKitLoadStatus ev = webkit_web_view_get_load_status (webview);
    log << debug << "tv: on_load_changed: " << ev << endl;
    switch (ev) {
      case WEBKIT_LOAD_FINISHED:
        log << debug << "tv: load finished." << endl;
        {

          /* load css style */
          GError *err = NULL;
          WebKitDOMDocument *d = webkit_web_view_get_dom_document (webview);
          WebKitDOMElement  *e = webkit_dom_document_create_element (d, STYLE_NAME, &err);

          WebKitDOMText *t = webkit_dom_document_create_text_node
            (d, thread_view_css.c_str());

          webkit_dom_node_append_child (WEBKIT_DOM_NODE(e), WEBKIT_DOM_NODE(t), (err = NULL, &err));

          WebKitDOMHTMLHeadElement * head = webkit_dom_document_get_head (d);
          webkit_dom_node_append_child (WEBKIT_DOM_NODE(head), WEBKIT_DOM_NODE(e), (err = NULL, &err));

          /* get container for message divs */
          container = WEBKIT_DOM_HTML_DIV_ELEMENT(webkit_dom_document_get_element_by_id (d, "message_container"));


          g_object_unref (d);
          g_object_unref (e);
          g_object_unref (t);
          g_object_unref (head);

          /* render */
          wk_loaded = true;
          render_post ();

        }
      default:
        break;
    }

    return true;
  }

  /*
   * Web inspector
   */
  extern "C" WebKitWebView * ThreadView_activate_inspector (
      WebKitWebInspector * wi,
      WebKitWebView *          w,
      gpointer                 data )
  {
    return ((ThreadView *) data)->activate_inspector (wi, w);
  }


  WebKitWebView * ThreadView::activate_inspector (
      WebKitWebInspector     * web_inspector,
      WebKitWebView          * web_view)
  {
    /* set up inspector window */
    log << info << "tv: starting conversation inspector.." << endl;
    inspector_window = new Gtk::Window ();
    inspector_window->set_default_size (600, 600);
    inspector_window->set_title (ustring::compose("Conversation inspector: %1", thread->subject));
    inspector_window->add (inspector_scroll);
    WebKitWebView * inspector_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    gtk_container_add (GTK_CONTAINER (inspector_scroll.gobj()),
          GTK_WIDGET (inspector_view));

    return inspector_view;
  }

  extern "C" bool ThreadView_show_inspector (
      WebKitWebInspector * wi,
      gpointer data)
  {
    return ((ThreadView *) data)->show_inspector (wi);
  }

  bool ThreadView::show_inspector (WebKitWebInspector * wi) {
    log << info << "tv: show inspector.." << endl;

    inspector_window->show_all ();

    release_modal ();

    return true;
  }

  /*
   * Inspector end
   */

  void ThreadView::load_thread (refptr<NotmuchThread> _thread) {
    log << info << "tv: load thread: " << _thread->thread_id << endl;
    thread = _thread;

    ((Gtk::Label*) tab_widget)->set_text (thread->thread_id);

    Db db (Db::DbMode::DATABASE_READ_ONLY);
    lock_guard<Db> grd (db);

    auto _mthread = refptr<MessageThread>(new MessageThread (thread));
    _mthread->load_messages (&db);
    load_message_thread (_mthread);
  }

  void ThreadView::load_message_thread (refptr<MessageThread> _mthread) {
    mthread = _mthread;

    /* remove unread status */
    if (mthread->in_notmuch && mthread->thread->has_tag ("unread")) {
      Db db (Db::DbMode::DATABASE_READ_WRITE);
      main_window->actions.doit (&db, refptr<Action>(new TagAction(mthread->thread, {}, {"unread"})));
    }

    ustring s = mthread->subject;
    if (static_cast<int>(s.size()) > MAX_TAB_SUBJECT_LEN)
      s = s.substr(0, MAX_TAB_SUBJECT_LEN - 3) + "...";

    ((Gtk::Label*)tab_widget)->set_text (s);


    render ();
  }

  void ThreadView::render () {
    log << info << "render: loading html.." << endl;
    webkit_web_view_load_html_string (webview, thread_view_html.c_str (), "/tmp/");
  }

  void ThreadView::render_post () {
    log << debug << "render: html loaded, building messages.." << endl;
    if (!container || !wk_loaded) {
      log << error << "tv: div container and web kit not loaded." << endl;
      return;
    }

    for_each (mthread->messages.begin(),
              mthread->messages.end(),
              [&](refptr<Message> m) {
                add_message (m);
              });

    if (!focused_message) {
      if (!candidate_startup) {
        focused_message = mthread->messages[mthread->messages.size()-1];
        toggle_hidden (focused_message);

      } else {
        focused_message = candidate_startup;
      }
    }

    update_focus_status ();
  }

  void ThreadView::add_message (refptr<Message> m) {
    log << debug << "tv: adding message: " << m->mid << endl;

    ustring div_id = "message_" + m->mid;

    WebKitDOMNode * insert_before = webkit_dom_node_get_last_child (
        WEBKIT_DOM_NODE (container));

    WebKitDOMHTMLElement * div_message = make_message_div ();

    GError * err = NULL;
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT(div_message),
        "id", div_id.c_str(), &err);

    /* insert message div */
    webkit_dom_node_insert_before (WEBKIT_DOM_NODE(container),
        WEBKIT_DOM_NODE(div_message),
        insert_before,
        (err = NULL, &err));

    set_message_html (m, div_message);

    /* insert attachments */
    insert_attachments (m, div_message);

    /* optionally hide / collapse the message */
    if (!(has (m->tags, ustring("unread")) || has(m->tags, ustring("flagged")))) {
      WebKitDOMDOMTokenList * class_list =
        webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(div_message));

      webkit_dom_dom_token_list_add (class_list, "hide",
          (err = NULL, &err));

      g_object_unref (class_list);
    } else {
      if (!candidate_startup)
        candidate_startup = m;
    }

    /* focus first unread message */
    if (!focused_message) {
      if (has (m->tags, ustring("unread"))) {
        focused_message = m;
      }
    }

    g_object_unref (insert_before);
    g_object_unref (div_message);
  }

  void ThreadView::set_message_html (
      refptr<Message> m,
      WebKitDOMHTMLElement * div_message)
  {
    GError *err;

    /* load message into div */
    ustring header;
    WebKitDOMHTMLElement * div_email_container =
      select (WEBKIT_DOM_NODE(div_message), "div.email_container");

    /* build header */
    Address a (m->sender);
    ustring from;
    if (a.valid()) {
      from = ustring::compose ("<a href=\"mailto:%3\"><b>%1</b> %2</a>",
          Glib::Markup::escape_text (a.name ()),
          Glib::Markup::escape_text (a.email ()),
          Glib::Markup::escape_text (a.full_address())
          );
    } else {
      from = ustring::compose ("<a href=\"mailto:%1\">%1</a>",
          Glib::Markup::escape_text (m->sender)
          );
    }
    header += create_header_row ("From: ", from, true, false);


    if (internet_address_list_length (m->to()) > 0) {
      AddressList tos (m->to());
      ustring to;

      bool first = true;

      for (Address &a : tos.addresses) {
        if (!first) {
          to += ", ";
        } else {
          first = false;
        }

        to += ustring::compose ("<a href=\"mailto:%3\">%1 (%2)</a>",
          Glib::Markup::escape_text (a.name ()),
          Glib::Markup::escape_text (a.email ()),
          Glib::Markup::escape_text (a.full_address())
          );
      }

      header += create_header_row ("To: ", to, false, false);
    } else {
      insert_header_address (header, "To:", "", false);
    }

    if (internet_address_list_length (m->cc()) > 0) {
      insert_header_address (header, "Cc:",
          internet_address_list_to_string (m->cc(), false), false);
    }

    if (internet_address_list_length (m->bcc()) > 0) {
      insert_header_address (header, "Bcc:",
          internet_address_list_to_string (m->bcc(), false), false);
    }

    insert_header_date (header, m);

    if (m->subject.length() > 0) {
      insert_header_address (header, "Subject:", m->subject, false);

      WebKitDOMHTMLElement * subject = select (
          WEBKIT_DOM_NODE (div_message),
          ".header_container .subject");

      ustring s = Glib::Markup::escape_text(m->subject);
      if (static_cast<int>(s.size()) > MAX_PREVIEW_LEN)
        s = s.substr(0, MAX_PREVIEW_LEN - 3) + "...";

      webkit_dom_html_element_set_inner_html (subject, s.c_str(), (err = NULL, &err));

      g_object_unref (subject);
    }

    if (m->in_notmuch) {
      ustring tags_s = VectorUtils::concat_tags (m->tags);

      header += create_header_row ("Tags:", tags_s, false, true);


      WebKitDOMHTMLElement * tags = select (
          WEBKIT_DOM_NODE (div_message),
          ".header_container .tags");

      webkit_dom_html_element_set_inner_html (tags, Glib::Markup::escape_text(tags_s).c_str(), (err = NULL, &err));

      g_object_unref (tags);
    }


    /* insert header html*/
    WebKitDOMHTMLElement * table_header =
      select (WEBKIT_DOM_NODE(div_email_container),
          ".header_container .header");

    webkit_dom_html_element_set_inner_html (
        table_header,
        header.c_str(),
        (err = NULL, &err));

    /* build message body */
    WebKitDOMHTMLElement * span_body =
      select (WEBKIT_DOM_NODE(div_email_container), ".body");

    ustring body = m->viewable_text (true);

    WebKitDOMHTMLElement * preview = select (
        WEBKIT_DOM_NODE (div_message),
        ".header_container .preview");


    ustring bp = m->viewable_text (false);
    if (static_cast<int>(body.size()) > MAX_PREVIEW_LEN)
      bp = bp.substr(0, MAX_PREVIEW_LEN - 3) + "...";

    while (true) {
      size_t i = bp.find ("<br>");

      if (i == ustring::npos) break;

      bp.erase (i, 4);
    }

    bp = Glib::Markup::escape_text (bp);

    webkit_dom_html_element_set_inner_html (preview, bp.c_str(), (err = NULL, &err));
    g_object_unref (preview);


    MailQuotes::filter_quotes (body);

    webkit_dom_html_element_set_inner_html (
        span_body,
        body.c_str(),
        (err = NULL, &err));

    g_object_unref (span_body);
    g_object_unref (table_header);
  }

  void ThreadView::insert_header_date (ustring & header, refptr<Message> m)
  {

    ustring value = ustring::compose (
                "<span class=\"hidden_only\">%1</span>"
                "<span class=\"not_hidden_only\">%2</span>",
                m->pretty_date (),
                m->pretty_verbose_date ());

    header += create_header_row ("Date: ", value, true, false);
  }

  void ThreadView::insert_header_address (
      ustring &header,
      ustring title,
      ustring address,
      bool important) {

    header += create_header_row (title, address, important, true);

  }

  ustring ThreadView::create_header_row (
      ustring title,
      ustring value,
      bool important,
      bool escape) {

    return ustring::compose (
        "<div class=\"field %1\">"
        "  <div class=\"title\">%2</div>"
        "  <div class=\"value\">%3</div>"
        "</div>",
        (important ? "important" : ""),
        Glib::Markup::escape_text (title),
        (escape ? Glib::Markup::escape_text (value) : value)
        );

  }

  void ThreadView::insert_attachments (
      refptr<Message> message,
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

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMHTMLElement * attachment_container =
      clone_select (WEBKIT_DOM_NODE(d), "#attachment_template");
    WebKitDOMHTMLElement * attachment_template =
      select (WEBKIT_DOM_NODE(attachment_container), ".attachment");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (attachment_container),
        "id");
    webkit_dom_node_remove_child (WEBKIT_DOM_NODE (attachment_container),
        WEBKIT_DOM_NODE(attachment_template), (err = NULL, &err));

    int attachments = 0;

    /* generate an attachment table for each attachment */
    for (refptr<Chunk> &c : message->attachments ()) {
      WebKitDOMHTMLElement * attachment_table =
        clone_node (WEBKIT_DOM_NODE (attachment_template));

      attachments++;

      WebKitDOMHTMLElement * info_fname =
        select (WEBKIT_DOM_NODE (attachment_table), ".info .filename");

      ustring fname = c->get_filename ();
      if (fname.size () == 0) {
        fname = "Unnamed attachment";
      }

      fname = Glib::Markup::escape_text (fname);

      webkit_dom_html_element_set_inner_text (info_fname, fname.c_str(), (err = NULL, &err));

      WebKitDOMHTMLElement * info_fsize =
        select (WEBKIT_DOM_NODE (attachment_table), ".info .filesize");

      refptr<Glib::ByteArray> attachment_data = c->contents ();

      ustring fsize = ustring::compose ("%1 bytes", attachment_data->size());

      webkit_dom_html_element_set_inner_text (info_fsize, fsize.c_str(), (err = NULL, &err));


      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (attachment_table),
        "data-attachment-id", ustring::compose("%1", c->id).c_str(),
        (err = NULL, &err));

      // set image
      WebKitDOMHTMLImageElement * img =
        WEBKIT_DOM_HTML_IMAGE_ELEMENT(
        select (WEBKIT_DOM_NODE (attachment_table), ".preview img"));

      set_attachment_src (c, attachment_data, img);

      // add the attachment table
      webkit_dom_node_append_child (WEBKIT_DOM_NODE (attachment_container),
          WEBKIT_DOM_NODE (attachment_table), (err = NULL, &err));

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
  }

  void ThreadView::set_attachment_src (
      refptr<Chunk> c,
      refptr<Glib::ByteArray> data,
      WebKitDOMHTMLImageElement *img)
  {
    /* set the preview image or icon on the attachment display element */

    const char * _mtype = g_mime_content_type_get_media_type (c->content_type);
    ustring mime_type;
    if (_mtype == NULL) {
      mime_type = "application/octet-stream";
    } else {
      mime_type = ustring(g_mime_content_type_to_string (c->content_type));
    }

    log << debug << "tv: set attachment, mime_type: " << mime_type << endl;

    gchar * content;
    gsize   content_size;
    ustring image_content_type;

    if ((_mtype != NULL) && (ustring(_mtype) == "image")) {
      auto mis = Gio::MemoryInputStream::create ();
      mis->add_data (data->get_data (), data->size ());

      auto pb = Gdk::Pixbuf::create_from_stream_at_scale (mis, THUMBNAIL_WIDTH, -1, true, refptr<Gio::Cancellable>());

      pb = pb->apply_embedded_orientation ();

      pb->save_to_buffer (content, content_size, "png");
      image_content_type = "image/png";

      GError * gerr;
      WebKitDOMDOMTokenList * class_list =
        webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(img));
      /* set class  */
      webkit_dom_dom_token_list_add (class_list, "thumbnail",
          (gerr = NULL, &gerr));

    } else {

      /*
      const char * _gio_content_type = g_content_type_from_mime_type (mime_type.c_str());
      ustring gio_content_type;
      if (_gio_content_type == NULL) {
        gio_content_type = "application/octet-stream";
      } else {
        gio_content_type = ustring(_gio_content_type);
      }
      GIcon * icon = g_content_type_get_icon (gio_content_type.c_str());

      ustring icon_string;
      if (icon == NULL) {
        icon_string = "mail-attachment-symbolic";
      } else {
        icon_string = g_icon_to_string (icon);
        g_object_unref (icon);
      }

      if (icon_string.size() < 1) {
        icon_string = "mail-attachment-symbolic";
      }


      log << debug << "icon: " << icon_string << flush << endl;
      */

      // TODO: use guessed icon
      ustring icon_string = "mail-attachment-symbolic";

      Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
      Glib::RefPtr<Gdk::Pixbuf> pixbuf = theme->load_icon (
          icon_string,
          ATTACHMENT_ICON_WIDTH,
          Gtk::ICON_LOOKUP_USE_BUILTIN );

      pixbuf->save_to_buffer (content, content_size, "png"); // default type is png
      image_content_type = "image/png";

    }

    GError * err = NULL;
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
        assemble_data_uri (image_content_type, content, content_size).c_str(), &err);

  }

  string ThreadView::assemble_data_uri (ustring mime_type, gchar * &data, gsize len) {
    string base64 = "data:" + mime_type + ";base64," + Glib::Base64::encode (string(data, len));
    return base64;
  }

  WebKitDOMHTMLElement * ThreadView::make_message_div () {
    /* clone div from template in html file */
    WebKitDOMDocument *d = webkit_web_view_get_dom_document (webview);
    WebKitDOMHTMLElement *e = clone_select (WEBKIT_DOM_NODE(d),
        "#email_template");
    g_object_unref (d);
    return e;
  }

  /* clone html elements */
  WebKitDOMHTMLElement * ThreadView::clone_select (
      WebKitDOMNode * node,
      ustring         selector,
      bool            deep) {

    return clone_node (WEBKIT_DOM_NODE(select (node, selector)), deep);
  }

  WebKitDOMHTMLElement * ThreadView::clone_node (
      WebKitDOMNode * node,
      bool            deep) {

    return WEBKIT_DOM_HTML_ELEMENT(webkit_dom_node_clone_node (node, deep));
  }

  WebKitDOMHTMLElement * ThreadView::select (
      WebKitDOMNode * node,
      ustring         selector) {

    GError * gerr = NULL;
    WebKitDOMHTMLElement *e;

    if (WEBKIT_DOM_IS_DOCUMENT(node)) {
      e = WEBKIT_DOM_HTML_ELEMENT(
        webkit_dom_document_query_selector (WEBKIT_DOM_DOCUMENT(node),
                                            selector.c_str(),
                                            &gerr));
    } else {
      /* ..DOMElement */
      e = WEBKIT_DOM_HTML_ELEMENT(
        webkit_dom_element_query_selector (WEBKIT_DOM_ELEMENT(node),
                                            selector.c_str(),
                                            &gerr));
    }

    if (gerr != NULL)
      log << error << "tv: clone_s_s_err: " << gerr->message << endl;

    return e;
  }

  bool ThreadView::on_key_press_event (GdkEventKey *event) {
    switch (event->keyval) {

      case GDK_KEY_j:
      case GDK_KEY_J:
      case GDK_KEY_Down:
        {
          if (event->keyval == GDK_KEY_J || event->state & GDK_CONTROL_MASK) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_page_increment ());
            update_focus_to_view ();
          } else {
            auto adj = scroll.get_vadjustment ();
            double v = adj->get_value ();
            adj->set_value (adj->get_value() + adj->get_step_increment ());

            if (v == adj->get_value ()) {
              /* we're at the bottom, just move focus down */
              focus_next ();
            } else {
              update_focus_to_view ();
            }
          }
        }
        return true;

      case GDK_KEY_k:
      case GDK_KEY_K:
      case GDK_KEY_Up:
        {
          if (event->keyval == GDK_KEY_K || event->state & GDK_CONTROL_MASK) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_page_increment ());
            update_focus_to_view ();
          } else {
            auto adj = scroll.get_vadjustment ();
            if (adj->get_value () == adj->get_lower ()) {
              /* we're at the top, move focus up */
              focus_previous ();

            } else {
              adj->set_value (adj->get_value() - adj->get_step_increment ());
              update_focus_to_view ();
            }
          }
        }
        return true;


      case GDK_KEY_Page_Up:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());
          update_focus_to_view ();
        }
        return true;

      case GDK_KEY_Page_Down:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());
          update_focus_to_view ();
        }
        return true;

      case GDK_KEY_1:
      case GDK_KEY_Home:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_lower ());
          focused_message = mthread->messages[0];
          update_focus_status ();
        }
        return true;

      case GDK_KEY_0:
      case GDK_KEY_End:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_upper ());
          focused_message = mthread->messages[mthread->messages.size()-1];
          update_focus_status ();
        }
        return true;

      case GDK_KEY_Return:
        {
          if (edit_mode) return false;
          toggle_hidden ();
          return true;
        }

      case GDK_KEY_n:
        {
          focus_next ();
          scroll_to_message (focused_message);
          return true;
        }

      case GDK_KEY_p:
        {
          focus_previous ();
          scroll_to_message (focused_message);
          return true;
        }

      case GDK_KEY_r:
        {
          /* reply to currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ReplyMessage (main_window, focused_message));

            return true;
          }
        }

      case GDK_KEY_f:
        {
          /* forward currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ForwardMessage (main_window, focused_message));

            return true;
          }
        }

      case GDK_KEY_V:
        {
          /* view raw source of currently focused message */
          main_window->add_mode (new RawMessage (focused_message));

          return true;
        }
    }

    return false;
  }

  void ThreadView::update_focus_to_view () {
    /* check if currently focused message has gone out of focus
     * and update focus */
    if (edit_mode) {
      focused_message = refptr<Message>();
      return;
    }

    /* loop through elements from the top and test whether the top
     * of it is within the view
     */

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    auto adj = scroll.get_vadjustment ();
    double scrolled = adj->get_value ();
    double height   = adj->get_page_size (); // 0 when there is
                                             // no paging.

    //log << debug << "scrolled = " << scrolled << ", height = " << height << endl;

    /* check currently focused message */
    bool take_next = false;

    /* take first */
    if (!focused_message) {
      //log << debug << "tv: u_f_t_v: none focused, take first initially." << endl;
      focused_message = mthread->messages[0];
      update_focus_status ();
    }

    /* check if focused message is still visible */
    ustring mid = "message_" + focused_message->mid;

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    double clientY = webkit_dom_element_get_offset_top (e);
    double clientH = webkit_dom_element_get_client_height (e);

    g_object_unref (e);

    //log << debug << "y = " << clientY << endl;
    //log << debug << "h = " << clientH << endl;

    // height = 0 if there is no paging: all messages are in view.
    if ((height == 0) || ( (clientY <= (scrolled + height)) && ((clientY + clientH) >= scrolled) )) {
      //log << debug << "message: " << focused_message->date() << " still in view." << endl;
    } else {
      //log << debug << "message: " << focused_message->date() << " out of view." << endl;
      take_next = true;
    }


    /* find first message that is in view and update
     * focused status */
    if (take_next) {
      int focused_position = find (
          mthread->messages.begin (),
          mthread->messages.end (),
          focused_message) - mthread->messages.begin ();
      int cur_position = 0;

      bool found = false;
      bool redo_focus_tags = false;

      for (auto &m : mthread->messages) {
        ustring mid = "message_" + m->mid;

        WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

        double clientY = webkit_dom_element_get_offset_top (e);
        double clientH = webkit_dom_element_get_client_height (e);

        // log << debug << "y = " << clientY << endl;
        // log << debug << "h = " << clientH << endl;

        WebKitDOMDOMTokenList * class_list =
          webkit_dom_element_get_class_list (e);

        GError * gerr = NULL;

        /* search for the last message that is currently in view
         * if the focused message is now below / beyond the view.
         * otherwise, take first that is in view now. */

        if ((!found || cur_position < focused_position) &&
            ( (height == 0) || ((clientY <= (scrolled + height)) && ((clientY + clientH) >= scrolled)) ))
        {
          // log << debug << "message: " << m->date() << " now in view." << endl;

          if (found) redo_focus_tags = true;
          found = true;
          focused_message = m;

          /* set class  */
          webkit_dom_dom_token_list_add (class_list, "focused",
              (gerr = NULL, &gerr));

        } else {
          /* reset class */
          webkit_dom_dom_token_list_remove (class_list, "focused",
              (gerr = NULL, &gerr));
        }

        g_object_unref (class_list);
        g_object_unref (e);

        cur_position++;
      }

      g_object_unref (d);

      if (redo_focus_tags) update_focus_status ();
    }
  }

  void ThreadView::update_focus_status () {
    if (edit_mode) {
      focused_message = refptr<Message>();
    }

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    for (auto &m : mthread->messages) {
      ustring mid = "message_" + m->mid;

      WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

      WebKitDOMDOMTokenList * class_list =
        webkit_dom_element_get_class_list (e);

      GError * gerr = NULL;

      if (focused_message == m)
      {
        /* set class  */
        webkit_dom_dom_token_list_add (class_list, "focused",
            (gerr = NULL, &gerr));

      } else {
        /* reset class */
        webkit_dom_dom_token_list_remove (class_list, "focused",
            (gerr = NULL, &gerr));
      }

      g_object_unref (class_list);
      g_object_unref (e);
    }

    g_object_unref (d);
  }

  void ThreadView::toggle_hidden (refptr<Message> m) {
    if (!m) m = focused_message;
    ustring mid = "message_" + focused_message->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (e);

    GError * gerr = NULL;

    if (webkit_dom_dom_token_list_contains (class_list, "hide", (gerr = NULL, &gerr)))
    {
      /* reset class */
      webkit_dom_dom_token_list_remove (class_list, "hide",
          (gerr = NULL, &gerr));

    } else {
      /* set class  */
      webkit_dom_dom_token_list_add (class_list, "hide",
          (gerr = NULL, &gerr));
    }

    g_object_unref (class_list);
    g_object_unref (e);
    g_object_unref (d);
  }

  void ThreadView::focus_next () {
    log << debug << "tv: focus_next." << endl;

    if (edit_mode) return;

    int focused_position = find (
        mthread->messages.begin (),
        mthread->messages.end (),
        focused_message) - mthread->messages.begin ();

    if (focused_position < static_cast<int>((mthread->messages.size ()-1))) {
      focused_message = mthread->messages[focused_position + 1];
      update_focus_status ();
    }
  }

  void ThreadView::focus_previous () {
    log << debug << "tv: focus previous." << endl;

    if (edit_mode) return;

    int focused_position = find (
        mthread->messages.begin (),
        mthread->messages.end (),
        focused_message) - mthread->messages.begin ();

    if (focused_position > 0) {
      focused_message = mthread->messages[focused_position - 1];
      update_focus_status ();
    }
  }

  void ThreadView::scroll_to_message (refptr<Message> m) {
    focused_message = m;

    if (edit_mode) return;

    if (!focused_message) {
      log << warn << "tv: focusing: no message selected for focus." << endl;
      return;
    }

    log << debug << "tv: focusing: " << m->date () << endl;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    auto adj = scroll.get_vadjustment ();

    ustring mid = "message_" + m->mid;

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    double clientY = webkit_dom_element_get_offset_top (e);
    //double clientH = webkit_dom_element_get_client_height (e);

    g_object_unref (e);
    g_object_unref (d);

    adj->set_value (clientY);

    update_focus_status ();
  }

  void ThreadView::grab_focus () {
    //log << debug << "tv: grab focus" << endl;
    gtk_widget_grab_focus (GTK_WIDGET (webview));
  }

  void ThreadView::grab_modal () {
    add_modal_grab ();
    grab_focus ();

    //gtk_grab_add (GTK_WIDGET (webview));
    //gtk_widget_grab_focus (GTK_WIDGET (webview));
  }

  void ThreadView::release_modal () {
    remove_modal_grab ();
    //gtk_grab_remove (GTK_WIDGET (webview));
  }

}

