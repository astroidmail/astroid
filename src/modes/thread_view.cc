# include <iostream>
# include <fstream>
# include <atomic>
# include <vector>
# include <algorithm>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "thread_view.hh"
# include "main_window.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "db.hh"
# include "utils/utils.hh"
# include "actions/action.hh"
# include "actions/tag_action.hh"
# include "reply_message.hh"


using namespace std;

namespace Astroid {
  bool ThreadView::theme_loaded = false;
  const char * ThreadView::thread_view_html_f = "ui/thread-view.html";
  const char * ThreadView::thread_view_css_f  = "ui/thread-view.css";
  ustring ThreadView::thread_view_html;
  ustring ThreadView::thread_view_css;

  ThreadView::ThreadView (MainWindow * mw) {
    main_window = mw;
    tab_widget = new Gtk::Label ("");

    pack_start (scroll, true, true, 5);

    /* set up webkit web view (using C api) */
    webview = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    websettings = WEBKIT_WEB_SETTINGS (webkit_web_settings_new ());
    g_object_set (G_OBJECT(websettings),
        "enable-scripts", FALSE,
        "enable-java-applet", FALSE,
        "enable-plugins", FALSE,
        "auto-load-images", FALSE,
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
    cout << "tv: deconstruct." << endl;
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
    cout << "tv: starting conversation inspector.." << endl;
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
    cout << "tv: show inspector.." << endl;

    inspector_window->show_all ();

    release_modal ();

    return true;
  }

  /*
   * Inspector end
   */

  void ThreadView::load_thread (refptr<NotmuchThread> _thread) {
    thread = _thread;

    ((Gtk::Label*) tab_widget)->set_text (thread->thread_id);

    auto _mthread = refptr<MessageThread>(new MessageThread (thread));
    _mthread->load_messages ();
    load_message_thread (_mthread);
  }

  void ThreadView::load_message_thread (refptr<MessageThread> _mthread) {
    mthread = _mthread;

    /* remove unread status */
    if (mthread->in_notmuch) {
      main_window->actions.doit (refptr<Action>(new TagAction(mthread->thread, {}, {"unread"})));
    }

    ustring s = mthread->subject;
    if (static_cast<int>(s.size()) > MAX_TAB_SUBJECT_LEN)
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

    update_focus_status ();
  }

  void ThreadView::add_message (refptr<Message> m) {
    cout << "tv: adding message: " << m->mid << endl;

    if (!focused_message) {
      focused_message = m;
    }

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
    } else {
      insert_header_address (header, "To:", "", true);
    }

    if (internet_address_list_length (m->cc()) > 0) {
      insert_header_address (header, "Cc:",
          internet_address_list_to_string (m->cc(), false), true);
    }

    if (internet_address_list_length (m->bcc()) > 0) {
      insert_header_address (header, "Bcc:",
          internet_address_list_to_string (m->bcc(), false), true);
    }

    insert_header_address (header, "Date:", m->pretty_verbose_date (), true);
    insert_header_address (header, "Subject:", m->subject, true);

    if (m->in_notmuch) {
      ustring tags_s = VectorUtils::concat_tags (m->tags ());
      insert_header_address (header, "Tags:", tags_s, true);
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
          if (event->state & GDK_CONTROL_MASK) {
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
          if (event->state & GDK_CONTROL_MASK) {
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
    }

    return false;
  }

  void ThreadView::update_focus_to_view () {
    /* check if currently focused message has gone out of focus
     * and update focus */
    cout << "tv: update_focus_to_view" << endl;

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

    cout << "scrolled = " << scrolled << ", height = " << height << endl;

    /* check currently focused message */
    bool take_next = false;

    /* take first */
    if (!focused_message) {
      cout << "tv: u_f_t_v: none focused, take first initially." << endl;
      focused_message = mthread->messages[0];
      update_focus_status ();
    }

    /* check if focused message is still visible */
    ustring mid = "message_" + focused_message->mid;

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    double clientY = webkit_dom_element_get_offset_top (e);
    double clientH = webkit_dom_element_get_client_height (e);

    g_object_unref (e);

    cout << "y = " << clientY << endl;
    cout << "h = " << clientH << endl;

    // height = 0 if there is no paging: all messages are in view.
    if ((height == 0) || ( (clientY <= (scrolled + height)) && ((clientY + clientH) >= scrolled) )) {
      cout << "message: " << focused_message->date() << " still in view." << endl;
    } else {
      cout << "message: " << focused_message->date() << " out of view." << endl;
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

        cout << "y = " << clientY << endl;
        cout << "h = " << clientH << endl;

        WebKitDOMDOMTokenList * class_list =
          webkit_dom_element_get_class_list (e);

        GError * gerr = NULL;

        /* search for the last message that is currently in view
         * if the focused message is now below / beyond the view.
         * otherwise, take first that is in view now. */

        if ((!found || cur_position < focused_position) &&
            ( (height == 0) || ((clientY <= (scrolled + height)) && ((clientY + clientH) >= scrolled)) ))
        {
          cout << "message: " << m->date() << " now in view." << endl;

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

  void ThreadView::focus_next () {
    cout << "tv: focus_next." << endl;

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
    cout << "tv: focus previous." << endl;

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
      cout << "tv: focusing: no message selected for focus." << endl;
      return;
    }

    cout << "tv: focusing: " << m->date () << endl;

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

