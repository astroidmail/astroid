# include <iostream>
# include <fstream>
# include <atomic>
# include <vector>
# include <algorithm>

# include <gtkmm.h>
# include <webkit/webkit.h>
# include <gio/gio.h>

# include "thread_view.hh"
# include "web_inspector.hh"
# include "dom_utils.hh"
# include "theme.hh"

# include "main_window.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "db.hh"
# include "utils/utils.hh"
# include "utils/address.hh"
# include "utils/vector_utils.hh"
# include "utils/gravatar.hh"
# include "actions/action.hh"
# include "actions/tag_action.hh"
# include "modes/mode.hh"
# include "modes/reply_message.hh"
# include "modes/forward_message.hh"
# include "modes/raw_message.hh"
# include "modes/thread_index/thread_index.hh"
# include "log.hh"
# include "theme.hh"

using namespace std;

namespace Astroid {

  ThreadView::ThreadView (MainWindow * mw) : Mode (mw) { // {{{
    const ptree& config = astroid->config ("thread_view");
    indent_messages = config.get<bool> ("indent_messages");
    open_html_part_external = config.get<bool> ("open_html_part_external");
    open_external_link = config.get<string> ("open_external_link");

    enable_mathjax = config.get<bool> ("mathjax.enable");
    mathjax_uri_prefix = config.get<string> ("mathjax.uri_prefix");

    ustring mj_only_tags = config.get<string> ("mathjax.for_tags");
    if (mj_only_tags.length() > 0) {
      mathjax_only_tags = VectorUtils::split_and_trim (mj_only_tags, ",");
    }

    enable_code_prettify = config.get<bool> ("code_prettify.enable");
    enable_code_prettify_for_patches = config.get<bool> ("code_prettify.enable_for_patches");
    code_prettify_prefix = config.get<string> ("code_prettify.uri_prefix");

    ustring cp_only_tags = config.get<string> ("code_prettify.for_tags");
    if (cp_only_tags.length() > 0) {
      code_prettify_only_tags = VectorUtils::split_and_trim (cp_only_tags, ",");
    }

    code_prettify_code_tag = config.get<string> ("code_prettify.code_tag");

    enable_gravatar = config.get<bool>("gravatar.enable");

    ready = false;

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
        "enable-scripts", TRUE,
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

    wk_loaded = false;

    g_signal_connect (webview, "notify::load-status",
        G_CALLBACK(ThreadView_on_load_changed),
        (gpointer) this );


    add_events (Gdk::KEY_PRESS_MASK);

    /* set up ThreadViewInspector */
    thread_view_inspector.setup (this);

    /* navigation requests */
    g_signal_connect (webview, "navigation-policy-decision-requested",
        G_CALLBACK(ThreadView_navigation_request),
        (gpointer) this);

    g_signal_connect (webview, "new-window-policy-decision-requested",
        G_CALLBACK(ThreadView_navigation_request),
        (gpointer) this);

    g_signal_connect (webview, "resource-request-starting",
        G_CALLBACK(ThreadView_resource_request_starting),
        (gpointer) this);

    /* scrolled window */
    auto vadj = scroll.get_vadjustment ();
    vadj->signal_changed().connect (
        sigc::mem_fun (this, &ThreadView::on_scroll_vadjustment_changed));

    /* load attachment icon */
    ustring icon_string = "mail-attachment-symbolic";

    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    attachment_icon = theme->load_icon (
        icon_string,
        ATTACHMENT_ICON_WIDTH,
        Gtk::ICON_LOOKUP_USE_BUILTIN );

    /* load marked icon */
    marked_icon = theme->load_icon (
        "object-select-symbolic",
        ATTACHMENT_ICON_WIDTH,
        Gtk::ICON_LOOKUP_USE_BUILTIN );

    register_keys ();

    show_all_children ();

    astroid->actions->signal_thread_updated ().connect (
        sigc::mem_fun (this, &ThreadView::on_thread_updated));
  }

  // }}}

  ThreadView::~ThreadView () { // {{{
    log << debug << "tv: deconstruct." << endl;
    // TODO: possibly still some errors here in paned mode
    //g_object_unref (webview); // probably garbage collected since it has a parent widget
    //g_object_unref (websettings);
    if (container) g_object_unref (container);
  } // }}}

  /* navigation requests {{{ */
  extern "C" void ThreadView_resource_request_starting (
      WebKitWebView         *web_view,
      WebKitWebFrame        *web_frame,
      WebKitWebResource     *web_resource,
      WebKitNetworkRequest  *request,
      WebKitNetworkResponse *response,
      gpointer               user_data) {

    ((ThreadView*) user_data)->resource_request_starting (
      web_view,
      web_frame,
      web_resource,
      request,
      response);
  }

  void ThreadView::resource_request_starting (
      WebKitWebView         * /* web_view */,
      WebKitWebFrame        * /* web_frame */,
      WebKitWebResource     * /* web_resource */,
      WebKitNetworkRequest  * request,
      WebKitNetworkResponse * response) {

    if (response != NULL) {
      /* a previously handled request */
      return;
    }

    const gchar * uri_c = webkit_network_request_get_uri (request);
    ustring uri (uri_c);

    // prefix of local uris for loading image thumbnails
    vector<ustring> allowed_uris =
      {
        home_uri,
        "data:image/png;base64",
      };

    if (enable_gravatar) {
      allowed_uris.push_back ("http://www.gravatar.com/avatar/");
    }

    if (enable_mathjax) {
      allowed_uris.push_back (mathjax_uri_prefix);
    }

    if (enable_code_prettify) {
      allowed_uris.push_back (code_prettify_prefix);
    }

    // TODO: show cid type images and inline-attachments

    /* is this request allowed */
    if (find_if (allowed_uris.begin (), allowed_uris.end (),
          [&](ustring &a) {
            return (uri.substr (0, a.length ()) == a);
          }) != allowed_uris.end ())
    {

      return; // yes

    } else {
      if (show_remote_images) {
        // TODO: use an approved-url (like geary) to only allow imgs, not JS
        //       or other content.
        log << warn  << "tv: remote images allowed, approving _all_ requests: " << uri << endl;
        return; // yes
      } else {
        log << debug << "tv: request: denied: " << uri << endl;
        webkit_network_request_set_uri (request, "about:blank"); // no
      }
    }
  }

  void ThreadView::reload_images () {

    GError * err = NULL;
    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    for (auto &m : mthread->messages) {

      ustring div_id = "message_" + m->mid;
      WebKitDOMElement * me = webkit_dom_document_get_element_by_id (d, div_id.c_str());

      WebKitDOMNodeList * imgs = webkit_dom_element_query_selector_all (me, "img", (err = NULL, &err));

      gulong l = webkit_dom_node_list_get_length (imgs);
      for (gulong i = 0; i < l; i++) {

        WebKitDOMNode * in = webkit_dom_node_list_item (imgs, i);
        WebKitDOMElement * ine = WEBKIT_DOM_ELEMENT (in);

        if (ine != NULL) {
          gchar * src = webkit_dom_element_get_attribute (ine, "src");
          if (src != NULL) {
            webkit_dom_element_set_attribute (ine, "src", "", (err = NULL, &err));
            webkit_dom_element_set_attribute (ine, "src", src, (err = NULL, &err));
          }

          // TODO: cid type images or attachment references are not loaded
        }

        g_object_unref (in);
      }

      g_object_unref (imgs);
      g_object_unref (me);
    }

    g_object_unref (d);
  }

  extern "C" gboolean ThreadView_navigation_request (
      WebKitWebView * w,
      WebKitWebFrame * frame,
      WebKitNetworkRequest * request,
      WebKitWebNavigationAction * navigation_action,
      WebKitWebPolicyDecision * policy_decision,
      gpointer user_data) {

    return ((ThreadView*)user_data)->navigation_request (
        w,
        frame,
        request,
        navigation_action,
        policy_decision);
  }

  gboolean ThreadView::navigation_request (
      WebKitWebView * /* w */,
      WebKitWebFrame * /* frame */,
      WebKitNetworkRequest * request,
      WebKitWebNavigationAction * navigation_action,
      WebKitWebPolicyDecision * policy_decision) {

    if (webkit_web_navigation_action_get_reason (navigation_action)
        == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {

      webkit_web_policy_decision_ignore (policy_decision);

      const gchar * uri_c = webkit_network_request_get_uri (
          request );

      ustring uri (uri_c);
      log << info << "tv: navigating to: " << uri << endl;

      ustring scheme = Glib::uri_parse_scheme (uri);

      if (scheme == "mailto") {

        uri = uri.substr (scheme.length ()+1, uri.length () - scheme.length()-1);
        UstringUtils::trim(uri);

        main_window->add_mode (new EditMessage (main_window, uri));

      } else if (scheme == "id") {
        /* TODO: open thread (doesnt work yet) */
        main_window->add_mode (new ThreadIndex (main_window, uri));

      } else if (scheme == "http" || scheme == "https" || scheme == "ftp")
      {
        open_link (uri);
      } else {

        log << error << "tv: unknown uri scheme. not opening." << endl;
      }

      return true;

    } else {
      log << info << "tv: navigation action: " <<
        webkit_web_navigation_action_get_reason (navigation_action) << endl;


      const gchar * uri_c = webkit_network_request_get_uri (
          request );

      ustring uri (uri_c);
      log << info << "tv: navigating to: " << uri << endl;
    }

    return false;
  }

  void ThreadView::open_link (ustring uri) {
    log << debug << "tv: opening: " << uri << endl;

    Glib::Threads::Thread::create (
        sigc::bind (
          sigc::mem_fun (this, &ThreadView::do_open_link), uri));
  }

  void ThreadView::do_open_link (ustring uri) {
    vector<string> args = { open_external_link.c_str(), uri.c_str () };
    log << debug << "tv: spawning: " << args[0] << ", " << args[1] << endl;
    string stdout;
    string stderr;
    int    exitcode;
    try {
      Glib::spawn_sync ("",
                        args,
                        Glib::SPAWN_DEFAULT | Glib::SPAWN_SEARCH_PATH,
                        sigc::slot <void> (),
                        &stdout,
                        &stderr,
                        &exitcode
                        );

    } catch (Glib::SpawnError &ex) {
      log << error << "tv: exception while opening uri: " <<  ex.what () << endl;
    }

    ustring ustdout = ustring(stdout);
    for (ustring &l : VectorUtils::split_and_trim (ustdout, ustring("\n"))) {

      log << debug << l << endl;
    }

    ustring ustderr = ustring(stderr);
    for (ustring &l : VectorUtils::split_and_trim (ustderr, ustring("\n"))) {

      log << debug << l << endl;
    }

    if (exitcode != 0) {
      log << error << "tv: open link exited with code: " << exitcode << endl;
    }
  }

  /* end navigation requests }}} */

  /* message loading {{{ */
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
      GtkWidget *       /* w */,
      GParamSpec *      /* p */)
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
          WebKitDOMElement  *e = webkit_dom_document_create_element (d, theme.STYLE_NAME, &err);

          WebKitDOMText *t = webkit_dom_document_create_text_node
            (d, theme.thread_view_css.c_str());

          webkit_dom_node_append_child (WEBKIT_DOM_NODE(e), WEBKIT_DOM_NODE(t), (err = NULL, &err));

          WebKitDOMHTMLHeadElement * head = webkit_dom_document_get_head (d);
          webkit_dom_node_append_child (WEBKIT_DOM_NODE(head), WEBKIT_DOM_NODE(e), (err = NULL, &err));

          /* load mathjax if enabled */
          if (enable_mathjax) {
            bool only_tags_ok = false;
            if (mathjax_only_tags.size () > 0) {
              if (mthread->in_notmuch) {
                for (auto &t : mathjax_only_tags) {
                  if (mthread->thread->has_tag (t)) {
                    only_tags_ok = true;
                    break;
                  }
                }
              } else {
                /* enable for messages not in db */
                only_tags_ok = true;
              }
            } else {
              only_tags_ok = true;
            }

            if (only_tags_ok) {
              math_is_on = true;

              WebKitDOMElement * me = webkit_dom_document_create_element (d, "SCRIPT", (err = NULL, &err));

              ustring mathjax_uri = mathjax_uri_prefix + "MathJax.js";

              webkit_dom_element_set_attribute (me, "type", "text/javascript",
                  (err = NULL, &err));
              webkit_dom_element_set_attribute (me, "src", mathjax_uri.c_str(),
                  (err = NULL, &err));

              webkit_dom_node_append_child (WEBKIT_DOM_NODE(head), WEBKIT_DOM_NODE(me), (err = NULL, &err));


              g_object_unref (me);
            }
          }

          /* load code_prettify if enabled */
          if (enable_code_prettify) {
            bool only_tags_ok = false;
            if (code_prettify_only_tags.size () > 0) {
              if (mthread->in_notmuch) {
                for (auto &t : code_prettify_only_tags) {
                  if (mthread->thread->has_tag (t)) {
                    only_tags_ok = true;
                    break;
                  }
                }
              } else {
                /* enable for messages not in db */
                only_tags_ok = true;
              }
            } else {
              only_tags_ok = true;
            }

            if (only_tags_ok) {
              code_is_on = true;

              WebKitDOMElement * me = webkit_dom_document_create_element (d, "SCRIPT", (err = NULL, &err));

              ustring code_prettify_uri = code_prettify_prefix + "run_prettify.js";

              webkit_dom_element_set_attribute (me, "type", "text/javascript",
                  (err = NULL, &err));
              webkit_dom_element_set_attribute (me, "src", code_prettify_uri.c_str(),
                  (err = NULL, &err));

              webkit_dom_node_append_child (WEBKIT_DOM_NODE(head), WEBKIT_DOM_NODE(me), (err = NULL, &err));


              g_object_unref (me);
            }
          }

          /* get container for message divs */
          container = WEBKIT_DOM_HTML_DIV_ELEMENT(webkit_dom_document_get_element_by_id (d, "message_container"));

          if (container == NULL) {
            log << warn << "render: could not find container!" << endl;
          }


          g_object_unref (d);
          g_object_unref (e);
          g_object_unref (t);
          g_object_unref (head);

          /* render */
          wk_loaded = true;
          render_messages ();

        }
      default:
        break;
    }

    return true;
  }

  void ThreadView::load_thread (refptr<NotmuchThread> _thread) {
    log << info << "tv: load thread: " << _thread->thread_id << endl;
    thread = _thread;

    set_label (thread->thread_id);

    Db db (Db::DbMode::DATABASE_READ_ONLY);

    auto _mthread = refptr<MessageThread>(new MessageThread (thread));
    _mthread->load_messages (&db);
    load_message_thread (_mthread);
  }

  void ThreadView::load_message_thread (refptr<MessageThread> _mthread) {
    mthread = _mthread;

    ustring s = mthread->subject;

    set_label (s);

    render ();
  }

  void ThreadView::on_message_changed (
      Db * db,
      Message * m,
      Message::MessageChangedEvent me)
  {
    if (!edit_mode) { // edit mode doesn't show tags
      if (me == Message::MessageChangedEvent::MESSAGE_TAGS_CHANGED) {
        if (m->in_notmuch) {
          log << debug << "tv: got message updated." << endl;
          refptr<Message> rm = refptr<Message> (m);
          rm->reference ();

          message_refresh_tags (db, rm);
        }
      }
    }
  }

  void ThreadView::on_thread_updated (Db * db, ustring thread_id) {
    if (!edit_mode) { // edit mode doesn't show tags
      if (thread && thread_id == thread->thread_id) {
        log << debug << "tv: got thread updated." << endl;
        /* thread was updated, check for:
         * - changed tags
         * - check if something should be done becasue of changed tags
         * - messages have been added
         * - messages have been removed
         *
         * TODO:
         *
         * if anything happens before the webview is ready it will not be shown,
         * however, we don't want the unread tag to be removed before we get the
         * chance to scroll to and expand the correct messages.
         */

        for (auto &m : mthread->messages) {
          m->refresh (db);
          message_refresh_tags (db, m);
        }
      }
    }
  }

  void ThreadView::message_refresh_tags (Db *, refptr<Message> m) {

    if (!wk_loaded || !ready) return;

    ustring tags_s = VectorUtils::concat_tags (m->tags);

    ustring tags_s_c = ustring::compose ("<span style=\"color:#31587a !important\">%1</span>",
        header_row_value (tags_s, true));

    GError *err;
    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * div_message = webkit_dom_document_get_element_by_id (d, mid.c_str());


    WebKitDOMHTMLElement * tags = DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".header_container .tags");

    webkit_dom_html_element_set_inner_html (tags, header_row_value(tags_s, true).c_str(), (err = NULL, &err));

    g_object_unref (tags);

    tags = DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".header_container .header div#Tags .value");

    webkit_dom_html_element_set_inner_html (tags, tags_s_c.c_str (), (err = NULL, &err));

    g_object_unref (tags);
    g_object_unref (div_message);
    g_object_unref (d);
  }

  /* end message loading }}} */

  /* rendering {{{ */

  /* general message adding and rendering {{{ */
  void ThreadView::render () {
    log << info << "render: loading html.." << endl;
    if (container) g_object_unref (container);
    container = NULL;
    wk_loaded = false;

    /* home uri used for thread view - request will be relative this
     * non-existant (hopefully) directory. */
    home_uri = ustring::compose ("%1/%2",
        astroid->standard_paths ().config_dir.c_str(),
        UstringUtils::random_alphanumeric (120));

    webkit_web_view_load_html_string (webview, theme.thread_view_html.c_str (), home_uri.c_str());
    ready     = false;
  }

  void ThreadView::render_messages () {
    log << debug << "render: html loaded, building messages.." << endl;
    if (!container || !wk_loaded) {
      log << error << "tv: div container and web kit not loaded." << endl;
      return;
    }

    /* set message state vector */
    state.clear ();

    for_each (mthread->messages.begin(),
              mthread->messages.end(),
              [&](refptr<Message> m) {
                add_message (m);
                state.insert (std::pair<refptr<Message>, MessageState> (m, MessageState ()));

                if (!edit_mode) {
                  m->signal_message_changed ().connect (
                      sigc::mem_fun (this, &ThreadView::on_message_changed));
                }
              });

    update_all_indent_states ();

    if (!focused_message) {
      if (!candidate_startup) {
        log << debug << "tv: no message expanded, showing newest message." << endl;

        focused_message = *max_element (
            mthread->messages.begin (),
            mthread->messages.end (),
            [](refptr<Message> &a, refptr<Message> &b)
              {
                return ( a->received_time < b->received_time );
              });

        toggle_hidden (focused_message, ToggleShow);

      } else {
        focused_message = candidate_startup;
      }
    }

    scroll_to_message (focused_message, true);

    /* remove unread status:
     *
     * we do this here so that messages are can be exanded if they are unread,
     * before the tag is removed.
     *
     * TODO: it would be better to remove the unread tag once a message is focused
     *       or focused for say 2 seconds.
     *
     */
    if (mthread->in_notmuch && mthread->thread->has_tag ("unread")) {
      main_window->actions->doit (refptr<Action>(new TagAction(mthread->thread, {}, {"unread"})), false);
    }

    emit_ready ();
  }

  void ThreadView::update_all_indent_states () {
    for (auto &m : mthread->messages) {
      update_indent_state (m);
    }
  }

  void ThreadView::update_indent_state (refptr<Message> m) {

    ustring mid = "message_" + m->mid;
    GError * err = NULL;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    /* set indentation based on level */
    if (indent_messages && m->level > 0) {
      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (e),
          "style", ustring::compose ("margin-left: %1px", int(m->level * INDENT_PX)).c_str(), (err = NULL, &err));
    } else {
      webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (e), "style");
    }

    g_object_unref (e);
    g_object_unref (d);

  }

  void ThreadView::add_message (refptr<Message> m) {
    log << debug << "tv: adding message: " << m->mid << endl;

    ustring div_id = "message_" + m->mid;

    WebKitDOMNode * insert_before = webkit_dom_node_get_last_child (
        WEBKIT_DOM_NODE (container));

    WebKitDOMHTMLElement * div_message = DomUtils::make_message_div (webview);

    GError * err = NULL;
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT(div_message),
        "id", div_id.c_str(), &err);

    /* insert message div */
    webkit_dom_node_insert_before (WEBKIT_DOM_NODE(container),
        WEBKIT_DOM_NODE(div_message),
        insert_before,
        (err = NULL, &err));

    set_message_html (m, div_message);

    /* insert mime messages */
    if (!m->missing_content) {
      insert_mime_messages (m, div_message);
    }

    /* insert attachments */
    if (!m->missing_content) {
      bool has_attachment = insert_attachments (m, div_message);

      /* add attachment icon */
      if (has_attachment) {
        set_attachment_icon (m, div_message);
      }
    }

    /* marked */
    load_marked_icon (m, div_message);

    if (!edit_mode) {
      /* optionally hide / collapse the message */
      if (!(has (m->tags, ustring("unread")) || has(m->tags, ustring("flagged")))) {

        /* hide message */
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
    } else {
      focused_message = m;
    }

    g_object_unref (insert_before);
    g_object_unref (div_message);
  } // }}}

  /* main message generation {{{ */
  void ThreadView::set_message_html (
      refptr<Message> m,
      WebKitDOMHTMLElement * div_message)
  {
    GError *err;

    /* load message into div */
    ustring header;
    WebKitDOMHTMLElement * div_email_container =
      DomUtils::select (WEBKIT_DOM_NODE(div_message), "div.email_container");

    /* build header */
    insert_header_address (header, "From", Address(m->sender), true);

    insert_header_address_list (header, "To", AddressList(m->to()), false);

    if (internet_address_list_length (m->cc()) > 0) {
      insert_header_address_list (header, "Cc", AddressList(m->cc()), false);
    }

    if (internet_address_list_length (m->bcc()) > 0) {
      insert_header_address_list (header, "Bcc", AddressList(m->bcc()), false);
    }

    insert_header_date (header, m);

    if (m->subject.length() > 0) {
      insert_header_row (header, "Subject", m->subject, false);

      WebKitDOMHTMLElement * subject = DomUtils::select (
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

      ustring tags_s_c = ustring::compose ("<span style=\"color:#31587a !important\">%1</span>",
          Glib::Markup::escape_text(tags_s));

      header += create_header_row ("Tags", tags_s_c, false, false, true);


      WebKitDOMHTMLElement * tags = DomUtils::select (
          WEBKIT_DOM_NODE (div_message),
          ".header_container .tags");

      webkit_dom_html_element_set_inner_html (tags, Glib::Markup::escape_text(tags_s).c_str(), (err = NULL, &err));

      g_object_unref (tags);
    }

    /* avatar */
    if (enable_gravatar) {
      auto se = Address(m->sender);

      WebKitDOMHTMLImageElement * av = WEBKIT_DOM_HTML_IMAGE_ELEMENT (
          DomUtils::select (
          WEBKIT_DOM_NODE (div_message),
          ".avatar"));

      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (av), "src",
          Gravatar::get_image_uri (se.email (), Gravatar::Default::RETRO, 48).c_str()
          , (err = NULL, &err));

      g_object_unref (av);
    }

    /* insert header html*/
    WebKitDOMHTMLElement * table_header =
      DomUtils::select (WEBKIT_DOM_NODE(div_email_container),
          ".header_container .header");

    webkit_dom_html_element_set_inner_html (
        table_header,
        header.c_str(),
        (err = NULL, &err));

    /* if message is missing body, set warning and don't add any content */

    WebKitDOMHTMLElement * span_body =
      DomUtils::select (WEBKIT_DOM_NODE(div_email_container), ".body");

    WebKitDOMHTMLElement * preview = DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".header_container .preview");

    {
      if (!edit_mode &&
           any_of (Db::draft_tags.begin (),
                   Db::draft_tags.end (),
                   [&](ustring t) {
                     return has (m->tags, t);
                   }))
      {

        /* set warning */
        set_warning (m, "This message is a draft, edit it with E or delete with D.");

      }
    }

    if (m->missing_content) {
      /* set preview */
      webkit_dom_html_element_set_inner_html (preview, "<i>Message content is missing.</i>", (err = NULL, &err));

      /* set warning */
      set_warning (m, "The message file is missing, only fields cached in the notmuch database are shown. Most likely your database is out of sync.");

      /* add an explenation to the body */
      GError *err;

      WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
      WebKitDOMHTMLElement * body_container =
        DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#body_template");

      webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (body_container),
          "id");

      webkit_dom_html_element_set_inner_html (
          body_container,
          "<i>Message content is missing.</i>",
          (err = NULL, &err));

      webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
          WEBKIT_DOM_NODE (body_container), (err = NULL, &err));

      g_object_unref (body_container);
      g_object_unref (d);

    } else {

      /* build message body */
      create_message_part_html (m, m->root, span_body, true);

      /* preview */
      log << debug << "tv: make preview.." << endl;

      ustring bp = m->viewable_text (false);
      if (static_cast<int>(bp.size()) > MAX_PREVIEW_LEN)
        bp = bp.substr(0, MAX_PREVIEW_LEN - 3) + "...";

      while (true) {
        size_t i = bp.find ("<br>");

        if (i == ustring::npos) break;

        bp.erase (i, 4);
      }

      bp = Glib::Markup::escape_text (bp);

      webkit_dom_html_element_set_inner_html (preview, bp.c_str(), (err = NULL, &err));
    }

    g_object_unref (preview);
    g_object_unref (span_body);
    g_object_unref (table_header);
  } // }}}

  /* generating message parts {{{ */
  void ThreadView::create_message_part_html (
      refptr<Message> message,
      refptr<Chunk> c,
      WebKitDOMHTMLElement * span_body,
      bool /* check_siblings */)
  {

    ustring mime_type;
    if (c->content_type) {
      mime_type = ustring(g_mime_content_type_to_string (c->content_type));
    } else {
      mime_type = "application/octet-stream";
    }

    log << debug << "create message part: " << c->id << " (siblings: " << c->siblings.size() << ") (kids: " << c->kids.size() << ")" <<
      " (attachment: " << c->attachment << ")" << " (viewable: " << c->viewable << ")" << " (mimetype: " << mime_type << ")" << endl;

    if (c->attachment) return;

    // TODO: redundant sibling checking

    /* check if we're the preferred sibling */
    bool use = false;

    if (c->siblings.size() >= 1) {
      /* todo: use last preferred sibling */
      if (c->preferred) {
        use = true;
      } else {
        use = false;
      }
    } else {
      use = true;
    }

    if (use) {
      if (c->viewable && c->preferred) {
        create_body_part (message, c, span_body);
      } else if (c->viewable) {
        create_sibling_part (message, c, span_body);
      }

      for (auto &k: c->kids) {
        create_message_part_html (message, k, span_body, true);
      }
    } else {
      create_sibling_part (message, c, span_body);
    }
  }

  void ThreadView::create_body_part (
      refptr<Message> message,
      refptr<Chunk> c,
      WebKitDOMHTMLElement * span_body)
  {
    // <span id="body_template" class="body_part"></span>

    log << debug << "create body part: " << c->id << endl;

    GError *err;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMHTMLElement * body_container =
      DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#body_template");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (body_container),
        "id");

    ustring body = c->viewable_text (true);

    if (code_is_on) {
      if (message->is_patch ()) {
        log << debug << "tv: message is patch, syntax highlighting." << endl;
        body.insert (0, code_start_tag);
        body.insert (body.length()-1, code_stop_tag);

      } else {
        filter_code_tags (body);
      }
    }

    webkit_dom_html_element_set_inner_html (
        body_container,
        body.c_str(),
        (err = NULL, &err));

    webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
        WEBKIT_DOM_NODE (body_container), (err = NULL, &err));

    g_object_unref (body_container);
    g_object_unref (d);
  }

  void ThreadView::filter_code_tags (ustring &body) {
    time_t t0 = clock ();
    ustring code_tag = code_prettify_code_tag;
    ustring start_tag = code_start_tag;
    ustring stop_tag  = code_stop_tag;

    if (code_tag.length() < 1) {
      throw runtime_error ("tv: cannot have a code tag with length 0");
    }

    /* search for matching code tags */
    ustring::size_type pos = 0;

    while (true) {
      /* find first */
      ustring::size_type first = body.find (code_tag, pos);

      if (first != ustring::npos) {
        /* find second */
        ustring::size_type second = body.find (code_tag, first + code_tag.length ());
        if (second != ustring::npos) {
          /* found matching tags */
          body.erase  (first, code_tag.length());
          body.insert (first, start_tag);

          second += start_tag.length () - code_tag.length ();

          body.erase  (second, code_tag.length ());
          body.insert (second, stop_tag);
          second += stop_tag.length () - code_tag.length ();

          pos = second;
        } else {
          /* could not find matching, done */
          break;
        }

      } else {
        /* done */
        break;
      }
    }

    log << debug << "tv: code filter done, time: " << ((clock() - t0) * 1000 / CLOCKS_PER_SEC) << " ms." << endl;

  }

  void ThreadView::display_part (refptr<Message> message, refptr<Chunk> c, MessageState::Element el) {
    GError * err;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    ustring mid = "message_" + message->mid;

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMHTMLElement * div_email_container =
      DomUtils::select (WEBKIT_DOM_NODE(e), "div.email_container");

    WebKitDOMHTMLElement * span_body =
      DomUtils::select (WEBKIT_DOM_NODE(div_email_container), ".body");

    WebKitDOMElement * sibling =
      webkit_dom_document_get_element_by_id (d, el.element_id().c_str());

    webkit_dom_node_remove_child (WEBKIT_DOM_NODE (span_body),
        WEBKIT_DOM_NODE(sibling), (err = NULL, &err));

    state[message].elements.erase (
        std::remove(
          state[message].elements.begin(),
          state[message].elements.end(), el),
        state[message].elements.end());

    state[message].current_element = 0;

    if (c->viewable) {
      create_body_part (message, c, span_body);
    }

    /* this shows parts that are nested directly below the part
     * as well. otherwise multipart/mixed with html's would
     * require two enters. */

    for (auto &k: c->kids) {
      if (k->viewable) {
        create_body_part (message, k, span_body);
      } else {
        create_message_part_html (message, k, span_body, true);
      }
    }

    g_object_unref (sibling);
    g_object_unref (span_body);
    g_object_unref (div_email_container);
    g_object_unref (e);
    g_object_unref (d);
  }

  void ThreadView::create_sibling_part (refptr<Message> message, refptr<Chunk> sibling, WebKitDOMHTMLElement * span_body) {

    log << debug << "create sibling part: " << sibling->id << endl;
    //
    //  <div id="sibling_template" class=sibling_container">
    //      <div class="top_border"></div>
    //      <div class="message"></div>
    //  </div>

    GError *err;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMHTMLElement * sibling_container =
      DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#sibling_template");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (sibling_container),
        "id");

    // add attachment to message state
    MessageState::Element e (MessageState::ElementType::Part, sibling->id);
    state[message].elements.push_back (e);
    log << debug << "tv: added sibling: " << state[message].elements.size() << endl;

    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (sibling_container),
      "id", e.element_id().c_str(),
      (err = NULL, &err));

    ustring content = ustring::compose ("Alternative part (type: %1) - potentially sketchy.",
        Glib::Markup::escape_text(sibling->get_content_type ()),
        e.element_id ());

    WebKitDOMHTMLElement * message_cont =
      DomUtils::select (WEBKIT_DOM_NODE (sibling_container), ".message");

    webkit_dom_html_element_set_inner_html (
        message_cont,
        content.c_str(),
        (err = NULL, &err));


    webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
        WEBKIT_DOM_NODE (sibling_container), (err = NULL, &err));

    g_object_unref (message_cont);
    g_object_unref (sibling_container);
    g_object_unref (d);
  } // }}}

  /* info and warning {{{ */
  void ThreadView::set_warning (refptr<Message> m, ustring txt)
  {
    log << debug << "tv: set warning: " << txt << endl;
    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMHTMLElement * warning = DomUtils::select (
        WEBKIT_DOM_NODE (e),
        ".email_warning");

    GError * err;
    webkit_dom_html_element_set_inner_html (warning, txt.c_str(), (err = NULL, &err));

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(warning));

    webkit_dom_dom_token_list_add (class_list, "show",
        (err = NULL, &err));

    g_object_unref (class_list);
    g_object_unref (warning);
    g_object_unref (e);
    g_object_unref (d);
  }

  void ThreadView::hide_warning (refptr<Message> m)
  {
    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMHTMLElement * warning = DomUtils::select (
        WEBKIT_DOM_NODE (e),
        ".email_warning");

    GError * err;
    webkit_dom_html_element_set_inner_html (warning, "", (err = NULL, &err));

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(warning));

    webkit_dom_dom_token_list_remove (class_list, "show",
        (err = NULL, &err));

    g_object_unref (class_list);
    g_object_unref (warning);
    g_object_unref (e);
    g_object_unref (d);
  }

  void ThreadView::set_info (refptr<Message> m, ustring txt)
  {
    log << debug << "tv: set info: " << txt << endl;

    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    if (e == NULL) {
      log << warn << "tv: could not get email div." << endl;
    }

    WebKitDOMHTMLElement * info = DomUtils::select (
        WEBKIT_DOM_NODE (e),
        ".email_info");

    GError * err;
    webkit_dom_html_element_set_inner_html (info, txt.c_str(), (err = NULL, &err));

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(info));

    webkit_dom_dom_token_list_add (class_list, "show",
        (err = NULL, &err));

    g_object_unref (class_list);
    g_object_unref (info);
    g_object_unref (e);
    g_object_unref (d);
  }

  void ThreadView::hide_info (refptr<Message> m) {
    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    if (e == NULL) {
      log << warn << "tv: could not get email div." << endl;
    }

    WebKitDOMHTMLElement * info = DomUtils::select (
        WEBKIT_DOM_NODE (e),
        ".email_info");

    GError * err;
    webkit_dom_html_element_set_inner_html (info, "", (err = NULL, &err));

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(info));

    webkit_dom_dom_token_list_remove (class_list, "show",
        (err = NULL, &err));

    g_object_unref (class_list);
    g_object_unref (info);
    g_object_unref (e);
    g_object_unref (d);
  }
  /* end info and warning }}} */

  /* headers {{{ */
  void ThreadView::insert_header_date (ustring & header, refptr<Message> m)
  {
    ustring value = ustring::compose (
                "<span class=\"hidden_only\">%1</span>"
                "<span class=\"not_hidden_only\">%2</span>",
                m->pretty_date (),
                m->pretty_verbose_date (true));

    header += create_header_row ("Date", value, true, false);
  }

  void ThreadView::insert_header_address (
      ustring &header,
      ustring title,
      Address address,
      bool important) {

    AddressList al (address);

    insert_header_address_list (header, title, al, important);
  }

  void ThreadView::insert_header_address_list (
      ustring &header,
      ustring title,
      AddressList addresses,
      bool important) {

    ustring value;
    bool first = true;

    for (Address &address : addresses.addresses) {
      if (address.full_address().size() > 0) {
        if (!first) {
          value += ", ";
        } else {
          first = false;
        }

        value +=
          ustring::compose ("<a href=\"mailto:%3\">%4%1%5 (%2)</a>",
            Glib::Markup::escape_text (address.fail_safe_name ()),
            Glib::Markup::escape_text (address.email ()),
            Glib::Markup::escape_text (address.full_address()),
            (important ? "<b>" : ""),
            (important ? "</b>" : "")
            );
      }
    }

    header += create_header_row (title, value, important, false);
  }

  void ThreadView::insert_header_row (
      ustring &header,
      ustring title,
      ustring value,
      bool important) {

    header += create_header_row (title, value, important, true);

  }


  ustring ThreadView::create_header_row (
      ustring title,
      ustring value,
      bool important,
      bool escape,
      bool noprint) {

    return ustring::compose (
        "<div class=\"field %1 %2\" id=\"%3\">"
        "  <div class=\"title\">%3:</div>"
        "  <div class=\"value\">%4</div>"
        "</div>",
        (important ? "important" : ""),
        (noprint ? "noprint" : ""),
        Glib::Markup::escape_text (title),
        header_row_value (value, escape)
        );
  }

  ustring ThreadView::header_row_value (ustring value, bool escape)  {
    return (escape ? Glib::Markup::escape_text (value) : value);
  }

  /* headers end }}} */

  /* attachments {{{ */
  void ThreadView::set_attachment_icon (
      refptr<Message> /* message */,
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

    /* set class  */
    webkit_dom_dom_token_list_add (class_list, "attachment",
        (err = NULL, &err));

    g_object_unref (class_list);
    g_object_unref (attachment_icon_img);
  }


  bool ThreadView::insert_attachments (
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
      DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#attachment_template");
    WebKitDOMHTMLElement * attachment_template =
      DomUtils::select (WEBKIT_DOM_NODE(attachment_container), ".attachment");

    webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (attachment_container),
        "id");
    webkit_dom_node_remove_child (WEBKIT_DOM_NODE (attachment_container),
        WEBKIT_DOM_NODE(attachment_template), (err = NULL, &err));

    int attachments = 0;

    /* generate an attachment table for each attachment */
    for (refptr<Chunk> &c : message->attachments ()) {
      WebKitDOMHTMLElement * attachment_table =
        DomUtils::clone_node (WEBKIT_DOM_NODE (attachment_template));

      attachments++;

      WebKitDOMHTMLElement * info_fname =
        DomUtils::select (WEBKIT_DOM_NODE (attachment_table), ".info .filename");

      ustring fname = c->get_filename ();
      if (fname.size () == 0) {
        fname = "Unnamed attachment";
      }

      fname = Glib::Markup::escape_text (fname);

      webkit_dom_html_element_set_inner_text (info_fname, fname.c_str(), (err = NULL, &err));

      WebKitDOMHTMLElement * info_fsize =
        DomUtils::select (WEBKIT_DOM_NODE (attachment_table), ".info .filesize");

      refptr<Glib::ByteArray> attachment_data = c->contents ();

      ustring fsize = Utils::format_size (attachment_data->size ());

      webkit_dom_html_element_set_inner_text (info_fsize, fsize.c_str(), (err = NULL, &err));


      // add attachment to message state
      MessageState::Element e (MessageState::ElementType::Attachment, c->id);
      state[message].elements.push_back (e);
      log << debug << "tv: added attachment: " << state[message].elements.size() << endl;

      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (attachment_table),
        "data-attachment-id", e.element_id().c_str(),
        (err = NULL, &err));
      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (attachment_table),
        "id", e.element_id().c_str(),
        (err = NULL, &err));

      // set image
      WebKitDOMHTMLImageElement * img =
        WEBKIT_DOM_HTML_IMAGE_ELEMENT(
        DomUtils::select (WEBKIT_DOM_NODE (attachment_table), ".preview img"));

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

    return (attachments > 0);
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

    log << debug << "tv: set attachment, mime_type: " << mime_type << ", mtype: " << _mtype << endl;

    gchar * content;
    gsize   content_size;
    ustring image_content_type;

    if ((_mtype != NULL) && (ustring(_mtype) == "image")) {
      auto mis = Gio::MemoryInputStream::create ();
      mis->add_data (data->get_data (), data->size ());

      try {

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

        g_object_unref (class_list);

      } catch (Gdk::PixbufError &ex) {

        log << error << "tv: could not create icon from attachmed image." << endl;

        attachment_icon->save_to_buffer (content, content_size, "png"); // default type is png
        image_content_type = "image/png";

      }
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

      attachment_icon->save_to_buffer (content, content_size, "png"); // default type is png
      image_content_type = "image/png";

    }

    GError * err = NULL;
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
        DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str(), &err);

  }
  /* attachments end }}} */

  /* marked {{{ */
  void ThreadView::load_marked_icon (
      refptr<Message> /* message */,
      WebKitDOMHTMLElement * div_message)
  {
    GError *err;

    WebKitDOMHTMLElement * marked_icon_img = DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".marked.icon.first");

    gchar * content;
    gsize   content_size;
    marked_icon->save_to_buffer (content, content_size, "png");
    ustring image_content_type = "image/png";

    WebKitDOMHTMLImageElement *img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (marked_icon_img);

    err = NULL;
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
        DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str(), &err);

    g_object_unref (marked_icon_img);
    marked_icon_img = DomUtils::select (
        WEBKIT_DOM_NODE (div_message),
        ".marked.icon.sec");
    img = WEBKIT_DOM_HTML_IMAGE_ELEMENT (marked_icon_img);
    err = NULL;
    webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (img), "src",
        DomUtils::assemble_data_uri (image_content_type, content, content_size).c_str(), &err);

    g_object_unref (marked_icon_img);
  }

  void ThreadView::update_marked_state (refptr<Message> m) {
    GError *err;
    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (e);

    /* set class  */
    if (state[m].marked) {
      webkit_dom_dom_token_list_add (class_list, "marked",
          (err = NULL, &err));
    } else {
      webkit_dom_dom_token_list_remove (class_list, "marked",
          (err = NULL, &err));
    }

    g_object_unref (class_list);
    g_object_unref (e);
    g_object_unref (d);

  }


  // }}}

  /* mime messages {{{ */
  void ThreadView::insert_mime_messages (
      refptr<Message> message,
      WebKitDOMHTMLElement * div_message)

  {
    WebKitDOMHTMLElement * div_email_container =
      DomUtils::select (WEBKIT_DOM_NODE(div_message), "div.email_container");

    WebKitDOMHTMLElement * span_body =
      DomUtils::select (WEBKIT_DOM_NODE(div_email_container), ".body");

    for (refptr<Chunk> &c : message->mime_messages ()) {
      log << debug << "create mime message part: " << c->id << endl;
      //
      //  <div id="mime_template" class=mime_container">
      //      <div class="top_border"></div>
      //      <div class="message"></div>
      //  </div>

      GError *err;

      WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
      WebKitDOMHTMLElement * mime_container =
        DomUtils::clone_select (WEBKIT_DOM_NODE(d), "#mime_template");

      webkit_dom_element_remove_attribute (WEBKIT_DOM_ELEMENT (mime_container),
          "id");

      // add attachment to message state
      MessageState::Element e (MessageState::ElementType::MimeMessage, c->id);
      state[message].elements.push_back (e);
      log << debug << "tv: added mime message: " << state[message].elements.size() << endl;

      webkit_dom_element_set_attribute (WEBKIT_DOM_ELEMENT (mime_container),
        "id", e.element_id().c_str(),
        (err = NULL, &err));

      ustring content = ustring::compose ("MIME message (subject: %1, size: %2 B) - potentially sketchy.",
          Glib::Markup::escape_text(c->get_filename ()),
          c->get_file_size (),
          e.element_id ());

      WebKitDOMHTMLElement * message_cont =
        DomUtils::select (WEBKIT_DOM_NODE (mime_container), ".message");

      webkit_dom_html_element_set_inner_html (
          message_cont,
          content.c_str(),
          (err = NULL, &err));


      webkit_dom_node_append_child (WEBKIT_DOM_NODE (span_body),
          WEBKIT_DOM_NODE (mime_container), (err = NULL, &err));

      g_object_unref (message_cont);
      g_object_unref (mime_container);
      g_object_unref (d);

    }

    g_object_unref (span_body);
    g_object_unref (div_email_container);

  }


  /* }}} */


  /* end rendering }}} */

  void ThreadView::register_keys () { // {{{
    keys.title = "Thread View";

    keys.register_key ("j", "thread_view.down",
        "Scroll down or move focus to next element",
        [&] (Key) {
          focus_next_element ();
          return true;
        });

    keys.register_key ("C-j", "thread_view.next_element",
        "Move focus to next element",
        [&] (Key) {
          /* move focus to next element and optionally scroll to it */

          ustring eid = focus_next_element (true);
          if (eid != "") {
            scroll_to_element (eid, false);
          }
          return true;
        });

    keys.register_key ("J", { Key (GDK_KEY_Down) }, "thread_view.scroll_down",
        "Scroll down",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          double v = adj->get_value ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());

          if (v < adj->get_value ()) {
            update_focus_to_view ();
          }

          return true;
        });

    keys.register_key ("C-d", { Key (true, false, (guint) GDK_KEY_Down), Key (GDK_KEY_Page_Down) },
        "thread_view.page_down",
        "Page down",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());
          update_focus_to_view ();
          return true;
        });

    keys.register_key ("k", "thread_view.up",
        "Scroll up or move focus to previous element",
        [&] (Key) {
          focus_previous_element ();
          return true;
        });

    keys.register_key ("C-k", "thread_view.previous_element",
        "Move focus to previous element",
        [&] (Key) {
          ustring eid = focus_previous_element (true);
          if (eid != "") {
            scroll_to_element (eid, false);
          }
          return true;
        });

    keys.register_key ("K", { Key (GDK_KEY_Up) },
        "thread_view.scroll_up",
        "Scroll up",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          if (!(adj->get_value () == adj->get_lower ())) {
            adj->set_value (adj->get_value() - adj->get_step_increment ());
            update_focus_to_view ();
          }
          return true;
        });

    keys.register_key ("C-u", { Key (true, false, (guint) GDK_KEY_Up), Key (GDK_KEY_Page_Up) },
        "thread_view.page_up",
        "Page up",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());
          update_focus_to_view ();
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) },
        "thread_view.home",
        "Scroll home",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_lower ());
          focused_message = mthread->messages[0];
          update_focus_status ();
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) },
        "thread_view.end",
        "Scroll to end",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_upper ());
          focused_message = mthread->messages[mthread->messages.size()-1];
          update_focus_status ();
          return true;
        });

    keys.register_key (Key (GDK_KEY_Return), { Key (GDK_KEY_KP_Enter) },
        "thread_view.activate",
        "Open/expand/activate focused element",
        [&] (Key) {
          return element_action (EEnter);
        });

    keys.register_key ("s", "thread_view.save",
        "Save attachment or message",
        [&] (Key) {
          return element_action (ESave);
        });

    keys.register_key ("d", "thread_view.delete",
        "Delete attachment (if editing)",
        [&] (Key) {
          if (edit_mode) {
            /* del attachment */
            return element_action (EDelete);
          }
          return false;
        });

    keys.register_key ("o", "thread_view.open",
        "Open attachment or message",
        [&] (Key) {
          return element_action (EOpen);
        });

    keys.register_key ("e", "thread_view.expand",
        "Toggle expand",
        [&] (Key) {
          if (edit_mode) return false;

          toggle_hidden ();

          return true;
        });

    keys.register_key ("C-e", "thread_view.toggle_expand_all",
        "Toggle expand on all messages",
        [&] (Key) {
          /* toggle hidden / shown status on all messages */
          if (edit_mode) return false;

          if (all_of (mthread->messages.begin(),
                      mthread->messages.end (),
                      [&](refptr<Message> m) {
                        return !is_hidden (m);
                      }
                )) {
            /* all are shown */
            for (auto m : mthread->messages) {
              toggle_hidden (m, ToggleHide);
            }

          } else {
            /* some are hidden */
            for (auto m : mthread->messages) {
              toggle_hidden (m, ToggleShow);
            }
          }

          return true;
        });

    keys.register_key ("t", "thread_view.mark",
        "Mark or unmark message",
        [&] (Key) {
          if (!edit_mode) {
            state[focused_message].marked = !(state[focused_message].marked);
            update_marked_state (focused_message);
            return true;
          }
          return false;
        });

    keys.register_key ("T", "thread_view.toggle_mark_all",
        "Toggle mark on all messages",
        [&] (Key) {
          if (!edit_mode) {
            for (auto &s : state) {
              s.second.marked = !s.second.marked;
              update_marked_state (s.first);
            }

            return true;
          }
          return false;
        });

    keys.register_key ("C-i", "thread_view.show_remote_images",
        "Show remote images (warning: approves all requests to remote content!)",
        [&] (Key) {
          show_remote_images = true;
          log << debug << "tv: show remote images: " << show_remote_images << endl;
          reload_images ();
          return true;
        });

    keys.register_key ("S", "thread_view.save_all_attachments",
        "Save all attachments",
        [&] (Key) {
          if (edit_mode) return false;
          save_all_attachments ();
          return true;
        });

    keys.register_key ("n", "thread_view.next_message",
        "Focus next message",
        [&] (Key) {
          focus_next ();
          scroll_to_message (focused_message);
          return true;
        });

    keys.register_key ("C-n", "thread_view.next_message_expand",
        "Focus next message (and expand if necessary)",
        [&] (Key) {
          if (state[focused_message].scroll_expanded) {
            toggle_hidden (focused_message, ToggleHide);
            state[focused_message].scroll_expanded = false;
            in_scroll = true;
          }

          focus_next ();

          if (is_hidden (focused_message)) {
            toggle_hidden (focused_message, ToggleShow);
            state[focused_message].scroll_expanded = true;
            in_scroll = true;
          }
          scroll_to_message (focused_message);
          return true;
        });

    keys.register_key ("p", "thread_view.previous_message",
        "Focus previous message",
        [&] (Key) {
          focus_previous ();
          scroll_to_message (focused_message);
          return true;
        });

    keys.register_key ("C-p", "thread_view.previous_message_expand",
        "Focus previous message (and expand if necessary)",
        [&] (Key) {
          if (state[focused_message].scroll_expanded) {
            toggle_hidden (focused_message, ToggleHide);
            state[focused_message].scroll_expanded = false;
            in_scroll = true;
          }

          focus_previous ();

          if (is_hidden (focused_message)) {
            toggle_hidden (focused_message, ToggleShow);
            state[focused_message].scroll_expanded = true;
            in_scroll = true;
          }
          scroll_to_message (focused_message);
          return true;
        });

    keys.register_key ("r", "thread_view.reply",
        "Reply to current message",
        [&] (Key) {
          /* reply to currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ReplyMessage (main_window, focused_message));

            return true;
          }
          return false;
        });

    keys.register_key ("G", "thread_view.reply_all",
        "Reply all to current message",
        [&] (Key) {
          /* reply to currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ReplyMessage (main_window, focused_message, ReplyMessage::ReplyMode::Rep_All));

            return true;
          }
          return false;
        });

    keys.register_key ("f", "thread_view.forward",
        "Forward current message",
        [&] (Key) {
          /* forward currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ForwardMessage (main_window, focused_message, ForwardMessage::FwdDisposition::FwdDefault));

            return true;
          }
          return false;
        });

    keys.register_key (UnboundKey (), "thread_view.forward_inline",
        "Forward current message inlined",
        [&] (Key) {
          /* forward currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ForwardMessage (main_window, focused_message, ForwardMessage::FwdDisposition::FwdInline));

            return true;
          }
          return false;
        });

    keys.register_key (UnboundKey (), "thread_view.forward_attached",
        "Forward current message as attachment",
        [&] (Key) {
          /* forward currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ForwardMessage (main_window, focused_message, ForwardMessage::FwdDisposition::FwdAttach));

            return true;
          }
          return false;
        });


    keys.register_key ("C-f", "thread_view.flat",
        "Toggle flat or indented view of messages",
        [&] (Key) {
          indent_messages = !indent_messages;
          update_all_indent_states ();
          return true;
        });

    keys.register_key ("V", "thread_view.view_raw",
        "View raw source for current message",
        [&] (Key) {
          /* view raw source of currently focused message */
          main_window->add_mode (new RawMessage (main_window, focused_message));

          return true;
        });

    keys.register_key ("E", "thread_view.edit_draft",
        "Edit currently focused message as new or draft",
        [&] (Key) {
          /* edit currently focused message as new or draft */
          if (!edit_mode) {
            main_window->add_mode (new EditMessage (main_window, focused_message));

            return true;
          }
          return false;
        });

    keys.register_key ("D", "thread_view.delete_draft",
        "Delete currently focused draft",
        [&] (Key) {
          if (!edit_mode) {

            if (any_of (Db::draft_tags.begin (),
                        Db::draft_tags.end (),
                        [&](ustring t) {
                          return has (focused_message->tags, t);
                        }))
            {
              ask_yes_no ("Do you want to delete this draft? (any changes will be lost)",
                  [&](bool yes) {
                    if (yes) {
                      EditMessage::delete_draft (focused_message);
                    }
                  });

              return true;
            }
          }
          return false;
        });

    multi_keys.register_key ("t", "thread_view.multi.toggle",
        "Toggle marked",
        [&] (Key) {
          for (auto &ms : state) {
            refptr<Message> m = ms.first;
            MessageState    s = ms.second;
            if (s.marked) {
              state[m].marked = false;
              update_marked_state (m);
            }
          }
          return true;
        });

    multi_keys.register_key ("s", "thread_view.multi.save",
        "Save marked",
        [&] (Key) {
          vector<refptr<Message>> tosave;

          for (auto &ms : state) {
            refptr<Message> m = ms.first;
            MessageState    s = ms.second;
            if (s.marked) {
              tosave.push_back (m);
            }
          }

          if (!tosave.empty()) {
            log << debug << "tv: saving messages: " << tosave.size() << endl;

            Gtk::FileChooserDialog dialog ("Save messages to folder..",
                Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

            dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
            dialog.add_button ("_Select", Gtk::RESPONSE_OK);

            int result = dialog.run ();

            switch (result) {
              case (Gtk::RESPONSE_OK):
                {
                  string dir = dialog.get_filename ();
                  log << info << "tv: saving messages to: " << dir << endl;

                  for (refptr<Message> m : tosave) {
                    m->save_to (dir);
                  }

                  break;
                }

              default:
                {
                  log << debug << "tv: save: cancelled." << endl;
                }
            }
          }

          return true;
        });

    multi_keys.register_key ("p", "thread_view.multi.print",
        "Print marked messages",
        [&] (Key) {
          vector<refptr<Message>> toprint;
          for (auto &ms : state) {
            refptr<Message> m = ms.first;
            MessageState    s = ms.second;
            if (s.marked) {
              toprint.push_back (m);
            }
          }

          GError * err = NULL;
          WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

          for (auto &m : toprint) {
            ustring mid = "message_" + m->mid;
            WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());
            WebKitDOMDOMTokenList * class_list =
              webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

            webkit_dom_dom_token_list_add (class_list, "print",
                (err = NULL, &err));

            /* expand */
            state[m].print_expanded = !toggle_hidden (m, ToggleShow);

          }

          bool indented = indent_messages;
          if (indent_messages) {
            indent_messages = false;
            update_all_indent_states ();
          }

          /* open print window */
          WebKitWebFrame * frame = webkit_web_view_get_main_frame (webview);
          webkit_web_frame_print (frame);

          if (indented) {
            indent_messages = true;
            update_all_indent_states ();
          }

          for (auto &m : toprint) {

            if (state[m].print_expanded) {
              toggle_hidden (m, ToggleHide);
              state[m].print_expanded = false;
            }

            ustring mid = "message_" + m->mid;
            WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());
            WebKitDOMDOMTokenList * class_list =
              webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

            webkit_dom_dom_token_list_remove (class_list, "print",
                (err = NULL, &err));

            g_object_unref (class_list);
            g_object_unref (e);
          }

          g_object_unref (d);

          return true;
        });

    keys.register_key ("+",
          "thread_view.multi",
          "Apply action to marked threads",
          [&] (Key k) {
            if (any_of (state.begin(), state.end(),
                  [](std::pair<refptr<Message>, ThreadView::MessageState> ms)
                  { return ms.second.marked; })
                )
            {

              multi_key (multi_keys, k);
            }

            return true;
          });

    keys.register_key ("C-P",
        "thread_view.print",
        "Print focused message",
        [&] (Key) {
          GError * err = NULL;
          WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

          ustring mid = "message_" + focused_message->mid;
          WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());
          WebKitDOMDOMTokenList * class_list =
            webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

          webkit_dom_dom_token_list_add (class_list, "print",
              (err = NULL, &err));

          /* expand */
          bool wasexpanded = toggle_hidden (focused_message, ToggleShow);

          bool indented = indent_messages;
          if (indent_messages) {
            indent_messages = false;
            update_all_indent_states ();
          }

          /* open print window */
          WebKitWebFrame * frame = webkit_web_view_get_main_frame (webview);
          webkit_web_frame_print (frame);

          if (indented) {
            indent_messages = true;
            update_all_indent_states ();
          }

          if (!wasexpanded) {
            toggle_hidden (focused_message, ToggleHide);
          }

          webkit_dom_dom_token_list_remove (class_list, "print",
              (err = NULL, &err));

          g_object_unref (class_list);
          g_object_unref (e);
          g_object_unref (d);

          return true;
        });

    keys.register_key ("l",
        "thread_view.tag_message",
        "Tag message",
        [&] (Key) {
          if (edit_mode) {
            return false;
          }

          ustring tag_list = VectorUtils::concat_tags (focused_message->tags) + ", ";

          main_window->enable_command (CommandBar::CommandMode::Tag,
              "Change tags for message:",
              tag_list,
              [&](ustring tgs) {
                log << debug << "ti: got tags: " << tgs << endl;

                vector<ustring> tags = VectorUtils::split_and_trim (tgs, ",");

                /* remove empty */
                tags.erase (std::remove (tags.begin (), tags.end (), ""), tags.end ());

                sort (tags.begin (), tags.end ());
                sort (focused_message->tags.begin (), focused_message->tags.end ());

                vector<ustring> rem;
                vector<ustring> add;

                /* find tags that have been removed */
                set_difference (focused_message->tags.begin (),
                                focused_message->tags.end (),
                                tags.begin (),
                                tags.end (),
                                std::back_inserter (rem));

                /* find tags that should be added */
                set_difference (tags.begin (),
                                tags.end (),
                                focused_message->tags.begin (),
                                focused_message->tags.end (),
                                std::back_inserter (add));


                if (add.size () == 0 &&
                    rem.size () == 0) {
                  log << debug << "ti: nothing to do." << endl;
                } else {
                  main_window->actions->doit (refptr<Action>(new TagAction (refptr<NotmuchTaggable>(new NotmuchMessage(focused_message)), add, rem)));
                }

              });
          return true;
        });

  }

  // }}}

  bool ThreadView::element_action (ElementAction a) { // {{{
    log << debug << "tv: activate item." << endl;

    if (!(focused_message)) {
      log << error << "tv: no message has focus yet." << endl;
      return true;
    }

    if (!edit_mode) {
      if (is_hidden (focused_message)) {
        if (a == EEnter) {
          toggle_hidden ();
        } else if (a == ESave) {
          /* save message to */
          focused_message->save ();
        }

      } else {
        if (state[focused_message].current_element == 0) {
          if (a == EEnter) {
            /* nothing selected, closing message */
            toggle_hidden ();
          } else if (a == ESave) {
            /* save message to */
            focused_message->save ();
          }
        } else {
          switch (state[focused_message].elements[state[focused_message].current_element].type) {
            case MessageState::ElementType::Attachment:
              {
                if (a == EEnter || a == EOpen) {
                  /* open attachment */

                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->open ();
                  } else {
                    log << error << "tv: could not find chunk for element." << endl;
                  }

                } else if (a == ESave) {
                  /* save attachment */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->save ();
                  } else {
                    log << error << "tv: could not find chunk for element." << endl;
                  }
                }
              }
              break;
            case MessageState::ElementType::Part:
              {
                if (a == EEnter || a == EOpen) {
                  /* open part */

                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    if (open_html_part_external) {
                      c->open ();
                    } else {
                      /* show part internally */

                      display_part (focused_message, c, state[focused_message].elements[state[focused_message].current_element]);

                    }
                  } else {
                    log << error << "tv: could not find chunk for element." << endl;
                  }

                } else if (a == ESave) {
                  /* save part */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->save ();
                  } else {
                    log << error << "tv: could not find chunk for element." << endl;
                  }
                }

              }
              break;

            case MessageState::ElementType::MimeMessage:
              {
                if (a == EEnter || a == EOpen) {
                  /* open part */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  refptr<MessageThread> mt = refptr<MessageThread> (new MessageThread ());
                  mt->add_message (c);

                  ThreadView * tv = new ThreadView (main_window);
                  tv->load_message_thread (mt);

                  main_window->add_mode (tv);

                } else if (a == ESave) {
                  /* save part */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->save ();
                  } else {
                    log << error << "tv: could not find chunk for element." << endl;
                  }
                }
              }
              break;

            default:
              break;
          }
        }
      }
    }

    if (state[focused_message].current_element > 0) {
      emit_element_action (state[focused_message].current_element, a);
    }
    return true;
  } // }}}

  /* focus handling {{{ */

  void ThreadView::on_scroll_vadjustment_changed () {
    if (in_scroll) {
      in_scroll = false;
      log << debug << "tv: re-doing scroll." << endl;

      if (scroll_arg != "") {
        if (scroll_to_element (scroll_arg, _scroll_when_visible)) {
          scroll_arg = "";
          _scroll_when_visible = false;
        }
      }
    }
  }

  void ThreadView::update_focus_to_view () {
    /* check if currently focused message has gone out of focus
     * and update focus */
    if (edit_mode) {
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
    /* update focus to currently set element (no scrolling ) */
    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    for (auto &m : mthread->messages) {
      ustring mid = "message_" + m->mid;

      WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

      WebKitDOMDOMTokenList * class_list =
        webkit_dom_element_get_class_list (e);

      GError * gerr = NULL;

      if (focused_message == m)
      {
        if (!edit_mode) {
          /* set class  */
          webkit_dom_dom_token_list_add (class_list, "focused",
              (gerr = NULL, &gerr));
        }

        /* check elements */
        unsigned int eno = 0;
        for (auto el : state[m].elements) {
          if (eno > 0) {

            WebKitDOMElement * ee = webkit_dom_document_get_element_by_id (d, el.element_id().c_str());

            WebKitDOMDOMTokenList * e_class_list =
              webkit_dom_element_get_class_list (ee);

            if (eno == state[m].current_element) {
              webkit_dom_dom_token_list_add (e_class_list, "focused",
                  (gerr = NULL, &gerr));

            } else {

              /* reset class */
              webkit_dom_dom_token_list_remove (e_class_list, "focused",
                  (gerr = NULL, &gerr));

            }

            g_object_unref (e_class_list);
            g_object_unref (ee);
          }

          eno++;
        }

      } else {
        /* reset class */
        if (!edit_mode) {
          webkit_dom_dom_token_list_remove (class_list, "focused",
              (gerr = NULL, &gerr));
        }

        /* check elements */
        unsigned int eno = 0;
        for (auto el : state[m].elements) {
          if (eno > 0) {

            WebKitDOMElement * ee = webkit_dom_document_get_element_by_id (d, el.element_id().c_str());

            WebKitDOMDOMTokenList * e_class_list =
              webkit_dom_element_get_class_list (ee);

            /* reset class */
            webkit_dom_dom_token_list_remove (e_class_list, "focused",
                (gerr = NULL, &gerr));

            g_object_unref (e_class_list);
            g_object_unref (ee);
          }

          eno++;
        }
      }

      g_object_unref (class_list);
      g_object_unref (e);
    }

    g_object_unref (d);
  }

  ustring ThreadView::focus_next_element (bool force_change) {
    ustring eid;

    if (!is_hidden (focused_message) || edit_mode) {
      /* if the message is expanded, check if we should move focus
       * to the next element */

      MessageState * s = &(state[focused_message]);

      //log << debug << "focus next: current element: " << s->current_element << " of " << s->elements.size() << endl;
      /* are there any more elements */
      if (s->current_element < (s->elements.size()-1)) {
        /* check if the next element is in full view */

        MessageState::Element * next_e = &(s->elements[s->current_element + 1]);

        WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

        auto adj = scroll.get_vadjustment ();

        eid = next_e->element_id ();

        bool change_focus = force_change;

        if (!force_change) {
          WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());

          double scrolled = adj->get_value ();
          double height   = adj->get_page_size (); // 0 when there is
                                                   // no paging.

          double clientY = webkit_dom_element_get_offset_top (e);
          double clientH = webkit_dom_element_get_client_height (e);

          g_object_unref (e);
          g_object_unref (d);

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

        //log << debug << "focus_next_element: change: " << change_focus << endl;

        if (change_focus) {
          s->current_element++;
          update_focus_status ();
          return eid;
        }
      }
    }

    /* no focus change, standard behaviour */
    auto adj = scroll.get_vadjustment ();
    double v = adj->get_value ();
    adj->set_value (adj->get_value() + adj->get_step_increment ());

    if (force_change  || (v == adj->get_value ())) {
      /* we're at the bottom, just move focus down */
      bool last = find (
          mthread->messages.begin (),
          mthread->messages.end (),
          focused_message) == (mthread->messages.end () - 1);


      if (!last) eid = focus_next ();
    } else {
      update_focus_to_view ();
    }

    return eid;
  }

  ustring ThreadView::focus_previous_element (bool force_change) {
    ustring eid;

    if (!is_hidden (focused_message) || edit_mode) {
      /* if the message is expanded, check if we should move focus
       * to the previous element */

      MessageState * s = &(state[focused_message]);

      //log << debug << "focus prev: current elemenet: " << s->current_element << endl;
      /* are there any more elements */
      if (s->current_element > 0) {
        /* check if the prev element is in full view */

        MessageState::Element * next_e = &(s->elements[s->current_element - 1]);

        bool change_focus = force_change;

        if (!force_change) {
          if (next_e->type != MessageState::ElementType::Empty) {
            WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

            auto adj = scroll.get_vadjustment ();

            eid = next_e->element_id ();

            WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());

            double scrolled = adj->get_value ();
            double height   = adj->get_page_size (); // 0 when there is
                                                     // no paging.

            double clientY = webkit_dom_element_get_offset_top (e);
            double clientH = webkit_dom_element_get_client_height (e);

            g_object_unref (e);
            g_object_unref (d);

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

        //log << debug << "focus_prev_element: change: " << change_focus << endl;

        if (change_focus) {
          s->current_element--;
          update_focus_status ();
          return eid;
        }
      }
    }

    /* standard behaviour */
    auto adj = scroll.get_vadjustment ();
    if (force_change || (adj->get_value () == adj->get_lower ())) {
      /* we're at the top, move focus up */
      ustring mid = focus_previous (false);
      ThreadView::MessageState::Element * e =
        state[focused_message].get_current_element ();
      if (e != NULL) {
        eid = e->element_id ();
      } else {
        eid = mid; // if no element selected, focus message
      }
    } else {
      adj->set_value (adj->get_value() - adj->get_step_increment ());
      update_focus_to_view ();
    }

    return eid;
  }

  ustring ThreadView::focus_next () {
    //log << debug << "tv: focus_next." << endl;

    if (edit_mode) return "";

    int focused_position = find (
        mthread->messages.begin (),
        mthread->messages.end (),
        focused_message) - mthread->messages.begin ();

    if (focused_position < static_cast<int>((mthread->messages.size ()-1))) {
      focused_message = mthread->messages[focused_position + 1];
      state[focused_message].current_element = 0; // start at top
      update_focus_status ();
    }

    return "message_" + focused_message->mid;
  }

  ustring ThreadView::focus_previous (bool focus_top) {
    //log << debug << "tv: focus previous." << endl;

    if (edit_mode) return "";

    int focused_position = find (
        mthread->messages.begin (),
        mthread->messages.end (),
        focused_message) - mthread->messages.begin ();

    if (!focus_top && focused_position > 0) {
      focused_message = mthread->messages[focused_position - 1];
      if (!is_hidden (focused_message)) {
        state[focused_message].current_element = state[focused_message].elements.size()-1; // start at bottom
      } else {
        state[focused_message].current_element = 0; // start at top
      }
      update_focus_status ();
    }

    return "message_" + focused_message->mid;
  }

  void ThreadView::scroll_to_message (refptr<Message> m, bool scroll_when_visible) {
    focused_message = m;

    if (edit_mode) return;

    if (!focused_message) {
      log << warn << "tv: focusing: no message selected for focus." << endl;
      return;
    }

    log << debug << "tv: focusing: " << m->date () << endl;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    ustring mid = "message_" + m->mid;

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    g_object_unref (e);
    g_object_unref (d);

    scroll_to_element (mid, scroll_when_visible);
  }

  bool ThreadView::scroll_to_element (
      ustring eid,
      bool scroll_when_visible)
  {
    /* returns false when rendering is incomplete and scrolling
     * doesn't work */

    if (eid == "") {
      log << error << "tv: attempted to scroll to unspecified id." << endl;
      return false;

    }

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, eid.c_str());

    auto adj = scroll.get_vadjustment ();
    double scrolled = adj->get_value ();
    double height   = adj->get_page_size (); // 0 when there is
                                             // no paging.
    double upper    = adj->get_upper ();

    double clientY = webkit_dom_element_get_offset_top (e);
    double clientH = webkit_dom_element_get_client_height (e);

    if (height > 0) {
      if (scroll_when_visible) {
        if ((clientY + clientH - height) > upper) {
          /* message is last, can't scroll past bottom */
          adj->set_value (upper);
        } else {
          adj->set_value (clientY);
        }
      } else {
        /* only scroll if parts of the message are out view */
        if (clientY < scrolled) {
          /* top is above view  */
          adj->set_value (clientY);

        } else if ((clientY + clientH) > (scrolled + height)) {
          /* bottom is below view */

          // if message is of less height than page, scroll so that
          // bottom is aligned with bottom

          if (clientH < height) {
            adj->set_value (clientY + clientH - height);
          } else {
            // otherwise align top with top
            if ((clientY + clientH - height) > upper) {
              /* message is last, can't scroll past bottom */
              adj->set_value (upper);
            } else {
              adj->set_value (clientY);
            }
          }
        }
      }
    }

    update_focus_status ();

    g_object_unref (e);
    g_object_unref (d);

    /* the height does not seem to make any sense, but is still more
     * than empty. we need to re-do the calculation when everything
     * has been rendered and re-calculated. */
    if (height == 1) {
      in_scroll = true;
      scroll_arg = eid;
      _scroll_when_visible = scroll_when_visible;
      return false;

    } else {

      return true;

    }
  }

  /* end focus handeling  }}} */

  /* message hiding {{{ */
  bool ThreadView::is_hidden (refptr<Message> m) {
    ustring mid = "message_" + m->mid;

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (e);

    GError * gerr = NULL;

    bool r = webkit_dom_dom_token_list_contains (class_list, "hide", (gerr = NULL, &gerr));

    g_object_unref (class_list);
    g_object_unref (e);
    g_object_unref (d);

    return r;
  }

  bool ThreadView::toggle_hidden (
      refptr<Message> m,
      ToggleState t)
  {
    /* returns true if the message was expanded in the first place */

    if (!m) m = focused_message;
    ustring mid = "message_" + m->mid;

    // reset element focus
    state[m].current_element = 0; // empty focus

    WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);

    WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());

    WebKitDOMDOMTokenList * class_list =
      webkit_dom_element_get_class_list (e);

    GError * gerr = NULL;

    bool wasexpanded;

    if (webkit_dom_dom_token_list_contains (class_list, "hide", (gerr = NULL, &gerr)))
    {
      wasexpanded = false;

      /* reset class */
      if (t == ToggleToggle || t == ToggleShow) {
        webkit_dom_dom_token_list_remove (class_list, "hide",
            (gerr = NULL, &gerr));
      }

    } else {
      wasexpanded = true;

      /* set class  */
      if (t == ToggleToggle || t == ToggleHide) {
        webkit_dom_dom_token_list_add (class_list, "hide",
            (gerr = NULL, &gerr));
      }
    }

    g_object_unref (class_list);
    g_object_unref (e);
    g_object_unref (d);

    return wasexpanded;
  }

  /* end message hinding }}} */

  void ThreadView::save_all_attachments () { // {{{
    /* save all attachments of current focused message */
    log << info << "tv: save all attachments.." << endl;

    if (!focused_message) {
      log << warn << "tv: no message focused!" << endl;
      return;
    }

    auto attachments = focused_message->attachments ();
    if (attachments.empty ()) {
      log << warn << "tv: this message has no attachments to save." << endl;
      return;
    }

    Gtk::FileChooserDialog dialog ("Save attachments to folder..",
        Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

    dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button ("_Select", Gtk::RESPONSE_OK);

    int result = dialog.run ();

    switch (result) {
      case (Gtk::RESPONSE_OK):
        {
          string dir = dialog.get_filename ();
          log << info << "tv: saving attachments to: " << dir << endl;

          /* TODO: check if the file exists and ask to overwrite. currently
           *       we are failing silently (except an error message in the log)
           */
          for (refptr<Chunk> a : attachments) {
            a->save_to (dir);
          }

          break;
        }

      default:
        {
          log << debug << "tv: save: cancelled." << endl;
        }
    }
  } // }}}

  /* general mode stuff {{{ */
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

  /* end general mode stuff }}} */

  /* signals {{{ */
  ThreadView::type_signal_ready
    ThreadView::signal_ready ()
  {
    return m_signal_ready;
  }

  void ThreadView::emit_ready () {
    log << info << "tv: ready emitted." << endl;
    m_signal_ready.emit ();
    ready = true;
  }

  ThreadView::type_element_action
    ThreadView::signal_element_action ()
  {
    return m_element_action;
  }

  void ThreadView::emit_element_action (unsigned int element, ElementAction action) {
    log << debug << "tv: element action emitted: " << element << ", action: enter" << endl;
    m_element_action.emit (element, action);
  }

  /* end signals }}} */

  /* MessageState  {{{ */
  ThreadView::MessageState::MessageState () {
    elements.push_back (Element (Empty, -1));
    current_element = 0;
  }

  ThreadView::MessageState::Element::Element (ThreadView::MessageState::ElementType t, int i)
  {
    type = t;
    id   = i;
  }

  bool ThreadView::MessageState::Element::operator== ( const Element & other ) const {
    return other.id == id;
  }

  ustring ThreadView::MessageState::Element::element_id () {
    return ustring::compose("%1", id);
  }

  ThreadView::MessageState::Element * ThreadView::MessageState::get_current_element () {
    if (current_element == 0) {
      return NULL;
    } else {
      return &(elements[current_element]);
    }
  }

  /* end MessageState }}} */
}

