# include <iostream>
# include <fstream>
# include <atomic>
# include <vector>
# include <algorithm>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "thread_view.hh"
# include "message_thread.hh"
# include "chunk.hh"


using namespace std;

namespace Astroid {
  bool ThreadView::theme_loaded = false;
  const char * ThreadView::thread_view_html_f = "ui/thread-view.html";
  const char * ThreadView::thread_view_css_f  = "ui/thread-view.css";
  ustring ThreadView::thread_view_html;
  ustring ThreadView::thread_view_css;


  ThreadView::ThreadView () {
    tab_widget = new Gtk::Label (thread_id);

    pack_start (scroll, true, true, 5);

    /* set up webkit web view (using C api) */
    webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    websettings = WEBKIT_WEB_SETTINGS (webkit_web_settings_new ());
    g_object_set (websettings, "enable-scripts", false);
    g_object_set (websettings, "enable-java-applet", false);
    g_object_set (websettings, "enable-plugins", false);
    g_object_set (websettings, "auto-load-images", false);
    g_object_set (websettings, "enable-display-of-insecure-content", false);
    g_object_set (websettings, "enable-dns-prefetching", false);
    g_object_set (websettings, "enable-fullscreen", false);
    g_object_set (websettings, "enable-html5-database", false);
    g_object_set (websettings, "enable-html5-local-storage", false);
    g_object_set (websettings, "enable-mediastream", false);
    g_object_set (websettings, "enable-mediasource", false);
    g_object_set (websettings, "enable-offline-web-application-cache", false);
    g_object_set (websettings, "enable-page-cache", false);
    g_object_set (websettings, "enable-private-browsing", true);
    g_object_set (websettings, "enable-running-of-insecure-content", false);
    g_object_set (websettings, "enable-xss-auditor", true);
    g_object_set (websettings, "media-playback-requires-user-gesture", true);

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
  }

  ThreadView::~ThreadView () {
    g_object_unref (webview);
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
    cout << "tv: on_load_changed: " << ev << endl;
    switch (ev) {
      case WEBKIT_LOAD_FINISHED:
        cout << "tv: load finished." << endl;
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
        break;
    }

    return true;
  }

  void ThreadView::load_thread (ustring _thread_id) {
    thread_id = _thread_id;

    ((Gtk::Label*) tab_widget)->set_text (thread_id);

    mthread = new MessageThread (thread_id);
    mthread->load_messages ();

    ustring s = mthread->subject;
    if (s.size() > MAX_TAB_SUBJECT_LEN)
      s = s.substr(0, MAX_TAB_SUBJECT_LEN - 3) + "...";

    ((Gtk::Label*)tab_widget)->set_text (s);


    render ();
  }

  void ThreadView::render () {
    cout << "render: loading html.." << endl;
    webkit_web_view_load_html_string (webview, thread_view_html.c_str (), "/tmp/");
  }

  void ThreadView::render_post () {
    cout << "render: html loaded, building messages.." << endl;
    if (!container || !wk_loaded) {
      cerr << "tv: div container and web kit not loaded." << endl;
      return;
    }

    for_each (mthread->messages.begin(),
              mthread->messages.end(),
              [&](refptr<Message> m) {
                add_message (m);
              });
  }

  void ThreadView::add_message (refptr<Message> m) {
    cout << "tv: adding message: " << m->mid << endl;

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
    insert_header_address (header, "From:", m->sender, true);

    if (internet_address_list_length (m->to()) > 0) {
      insert_header_address (header, "To:",
          internet_address_list_to_string (m->to(), false), true);
    }

    if (internet_address_list_length (m->cc()) > 0) {
      insert_header_address (header, "Cc:",
          internet_address_list_to_string (m->cc(), false), true);
    }

    if (internet_address_list_length (m->bcc()) > 0) {
      insert_header_address (header, "Bcc:",
          internet_address_list_to_string (m->bcc(), false), true);
    }

    insert_header_address (header, "Date:", m->date (), true);
    insert_header_address (header, "Subject:", m->subject, true);

    auto tags = m->tags ();
    ustring tags_s;
    for_each (tags.begin(), tags.end(),
        [&](ustring t) {
          if (t != *tags.begin()) {
            tags_s += ", ";
          }
          tags_s = tags_s + t;
        });

    insert_header_address (header, "Tags:", tags_s, true);


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

    refptr<Chunk> c = m->root;
    ustring body;

    function< void (refptr<Chunk>) > app_body =
      [&] (refptr<Chunk> c)
    {
      /* check if we're the preferred sibling */
      bool use = false;

      if (c->siblings.size() >= 1) {
        if (c->preferred) {
          use = true;
        } else {
          /* check if there are any other preferred */
          if (all_of (c->siblings.begin (),
                      c->siblings.end (),
                      [](refptr<Chunk> c) { return (!c->preferred); })) {
            use = true;
          } else {
            use = false;
          }
        }
      } else {
        use = true;
      }

      if (use) {
        if (c->viewable) body += c->body();

        for_each (c->kids.begin(),
                  c->kids.end (),
                  app_body);
      }
    };

    app_body (c);


    webkit_dom_html_element_set_inner_html (
        span_body,
        body.c_str(),
        (err = NULL, &err));

    g_object_unref (table_header);
  }

  void ThreadView::insert_header_address (
      ustring &header,
      ustring title,
      ustring address,
      bool important) {

    header += create_header_row (title, address, important);

  }

  ustring ThreadView::create_header_row (
      ustring title,
      ustring value,
      bool important) {

    return ustring::compose (
        "<div class=\"field %1\">"
        "  <div class=\"title\">%2</div>"
        "  <div class=\"value\">%3</div>"
        "</div>",
        (important ? "important" : ""),
        Glib::Markup::escape_text (title),
        Glib::Markup::escape_text (value)
        );

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
      cout << "tv: clone_s_s_err: " << gerr->message << endl;

    return e;
  }

  bool ThreadView::on_key_press_event (GdkEventKey *event) {
    cout << "tv: key press" << endl;
    switch (event->keyval) {

      case GDK_KEY_j:
      case GDK_KEY_J:
      case GDK_KEY_Down:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());
        }
        return true;

      case GDK_KEY_k:
      case GDK_KEY_K:
      case GDK_KEY_Up:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_step_increment ());
        }
        return true;


    }

    return false;
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

