# include <iostream>
# include <fstream>
# include <atomic>
# include <vector>
# include <algorithm>
# include <chrono>
# include <mutex>
# include <condition_variable>
# include <functional>

# include <gtkmm.h>
# include <webkit2/webkit2.h>
# include <gio/gio.h>
# include <boost/property_tree/ptree.hpp>

# include "build_config.hh"
# include "thread_view.hh"
# include "theme.hh"
# include "page_client.hh"

# include "main_window.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "crypto.hh"
# include "db.hh"
# include "utils/utils.hh"
# include "utils/address.hh"
# include "utils/vector_utils.hh"
# include "utils/ustring_utils.hh"
# include "utils/gravatar.hh"
# include "utils/cmd.hh"
# include "utils/gmime/gmime-compat.h"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif
# include "actions/action.hh"
# include "actions/cmdaction.hh"
# include "actions/tag_action.hh"
# include "actions/toggle_action.hh"
# include "actions/difftag_action.hh"
# include "modes/mode.hh"
# include "modes/reply_message.hh"
# include "modes/forward_message.hh"
# include "modes/raw_message.hh"
# include "modes/thread_index/thread_index.hh"
# include "theme.hh"

using namespace std;
using boost::property_tree::ptree;

namespace Astroid {

  ThreadView::ThreadView (MainWindow * mw, bool _edit_mode) : Mode (mw) { //
    edit_mode = _edit_mode;
    wk_loaded = false;
    ready = false;

    /* home uri used for thread view - request will be relative this
     * non-existant (hopefully) directory. */
    home_uri = ustring::compose ("%1/%2",
        astroid->standard_paths ().config_dir.c_str(),
        UstringUtils::random_alphanumeric (120));

    /* WebKit: set up webkit web view */

    /* create web context */
    context = webkit_web_context_new_ephemeral ();

    /* set up this extension interface */
    page_client = new PageClient (this);

    const ptree& config = astroid->config ("thread_view");
    indent_messages = config.get<bool> ("indent_messages");
    open_html_part_external = config.get<bool> ("open_html_part_external");
    open_external_link = config.get<string> ("open_external_link");

    expand_flagged = config.get<bool> ("expand_flagged");

    page_client->enable_gravatar = config.get<bool>("gravatar.enable");
    unread_delay = config.get<double>("mark_unread_delay");

    /* one process for each webview so that a new and unique
     * instance of the webextension is created for each webview
     * and page */
    webkit_web_context_set_process_model (context, WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);

    websettings = WEBKIT_SETTINGS (webkit_settings_new_with_settings (
        "enable-javascript", FALSE,
        "enable-java", FALSE,
        "enable-plugins", FALSE,
        "auto-load-images", TRUE,
        "enable-dns-prefetching", FALSE,
        "enable-fullscreen", FALSE,
        "enable-html5-database", FALSE,
        "enable-html5-local-storage", FALSE,
        "enable-mediasource", FALSE,
        "enable-offline-web-application-cache", FALSE,
        "enable-page-cache", FALSE,
        "enable-xss-auditor", TRUE,
        "media-playback-requires-user-gesture", TRUE,
        "zoom-text-only", TRUE,
        "enable-frame-flattening", TRUE,
# if (DEBUG || DEBUG_WEBKIT)
        "enable-developer-extras", TRUE,
# endif
        NULL));

    webview =
      WEBKIT_WEB_VIEW (
        g_object_new (WEBKIT_TYPE_WEB_VIEW,
          "web-context", context,
          "settings", websettings,
          NULL));

    if (g_object_is_floating (webview)) g_object_ref_sink (webview);

    gtk_box_pack_start (GTK_BOX (this->gobj ()), GTK_WIDGET (webview), true, true, 0);


    g_signal_connect (webview, "load-changed",
        G_CALLBACK(ThreadView_on_load_changed),
        (gpointer) this );


    add_events (Gdk::KEY_PRESS_MASK);

    g_signal_connect (webview, "decide-policy",
        G_CALLBACK(ThreadView_decide_policy),
        (gpointer) this);

    load_html ();

    register_keys ();

    show_all ();
    show_all_children ();

# ifndef DISABLE_PLUGINS
    /* load plugins */
    plugins = new PluginManager::ThreadViewExtension (this);
# endif

  }

  ThreadView::~ThreadView () { //
    LOG (debug) << "tv: deconstruct.";
    g_object_unref (context);
    g_object_unref (websettings);
    g_object_unref (webview);
  }

  void ThreadView::pre_close () {
# ifndef DISABLE_PLUGINS
    plugins->deactivate ();
    delete plugins;
# endif

    delete page_client;
  }

  /* navigation requests  */
  extern "C" gboolean ThreadView_decide_policy (
      WebKitWebView * w,
      WebKitPolicyDecision * decision,
      WebKitPolicyDecisionType decision_type,
      gpointer user_data) {

    return ((ThreadView *) user_data)->decide_policy (w, decision, decision_type);
  }

  gboolean ThreadView::decide_policy (
      WebKitWebView * /* w */,
      WebKitPolicyDecision *   decision,
      WebKitPolicyDecisionType decision_type)
  {

    LOG (debug) << "tv: decide policy";

    switch (decision_type) {
      case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION: // navigate to {{{
        {
          WebKitNavigationPolicyDecision * navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
          WebKitNavigationAction * nav_action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);

          if (webkit_navigation_action_get_navigation_type (nav_action)
              == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED) {

            webkit_policy_decision_ignore (decision);

            const gchar * uri_c = webkit_uri_request_get_uri (
                webkit_navigation_action_get_request (nav_action));


            ustring uri (uri_c);
            LOG (info) << "tv: navigating to: " << uri;

            ustring scheme = Glib::uri_parse_scheme (uri);

            if (scheme == "mailto") {

              uri = uri.substr (scheme.length ()+1, uri.length () - scheme.length()-1);
              UstringUtils::trim(uri);

              main_window->add_mode (new EditMessage (main_window, uri));

            } else if (scheme == "id" || scheme == "mid" ) {
              main_window->add_mode (new ThreadIndex (main_window, uri));

            } else if (scheme == "http" || scheme == "https" || scheme == "ftp") {
              open_link (uri);

            } else {

              LOG (error) << "tv: unknown uri scheme. not opening.";
            }
          }
        } // }}}
        break;

      case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
        webkit_policy_decision_ignore (decision);
        break;

      default:
        webkit_policy_decision_ignore (decision);
        return true; // stop event
    }

    return true; // stop event
  }

  void ThreadView::open_link (ustring uri) {
    LOG (debug) << "tv: opening: " << uri;

    Glib::Threads::Thread::create (
        sigc::bind (
          sigc::mem_fun (this, &ThreadView::do_open_link), uri));
  }

  void ThreadView::do_open_link (ustring uri) {
    vector<string> args = { open_external_link.c_str(), uri.c_str () };
    LOG (debug) << "tv: spawning: " << args[0] << ", " << args[1];
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
      LOG (error) << "tv: exception while opening uri: " <<  ex.what ();
    }

    ustring ustdout = ustring(stdout);
    for (ustring &l : VectorUtils::split_and_trim (ustdout, ustring("\n"))) {

      LOG (debug) << l;
    }

    ustring ustderr = ustring(stderr);
    for (ustring &l : VectorUtils::split_and_trim (ustderr, ustring("\n"))) {

      LOG (debug) << l;
    }

    if (exitcode != 0) {
      LOG (error) << "tv: open link exited with code: " << exitcode;
    }
  }

  /* end navigation requests  */

  /* message loading  */
  /*
   * By the C++ standard this callback-setup is not necessarily safe, but it seems
   * to be for both g++ and clang++.
   *
   * http://stackoverflow.com/questions/2068022/in-c-is-it-safe-portable-to-use-static-member-function-pointer-for-c-api-call
   *
   * http://gtk.10911.n7.nabble.com/Using-g-signal-connect-in-class-td57137.html
   *
   * To be portable we have to use a free function declared extern "C". A
   * static member function is likely to work at least on gcc/g++, but not
   * necessarily elsewhere.
   *
   */

  extern "C" bool ThreadView_on_load_changed (
      WebKitWebView * w,
      WebKitLoadEvent load_event,
      gpointer user_data)
  {
    return ((ThreadView *) user_data)->on_load_changed (w, load_event);
  }

  bool ThreadView::on_load_changed (
      WebKitWebView * /* w */,
      WebKitLoadEvent load_event)
  {
    LOG (debug) << "tv: on_load_changed: " << load_event;
    switch (load_event) {
      case WEBKIT_LOAD_FINISHED:
        LOG (debug) << "tv: load finished.";
        {
          /* render */
          wk_loaded = true;

          // also called in page_client
          if (page_client->ready) on_ready_to_render ();
        }
      default:
        break;
    }

    return true;
  }

  void ThreadView::load_thread (refptr<NotmuchThread> _thread) {
    LOG (info) << "tv: load thread: " << _thread->thread_id;
    thread = _thread;

    set_label (thread->thread_id);

    Db db (Db::DbMode::DATABASE_READ_ONLY);

    auto _mthread = refptr<MessageThread>(new MessageThread (thread));
    _mthread->load_messages (&db);

    if (unread_setup) unread_checker.disconnect ();
    unread_setup = false; // reset

    load_message_thread (_mthread);
  }

  void ThreadView::load_message_thread (refptr<MessageThread> _mthread) {
    ready = false;

    mthread.clear ();
    mthread = _mthread;

    if (wk_loaded && page_client->ready) {
      page_client->clear_messages ();
      render_messages (); // resets the state
    }

    ustring s = mthread->get_subject();
    set_label (s);
  }

  void ThreadView::on_message_changed (
      Db * /* db */,
      Message * m,
      Message::MessageChangedEvent me)
  {
    if (ready) {
      if (me == Message::MessageChangedEvent::MESSAGE_TAGS_CHANGED) {
        if (m->in_notmuch && m->tid == thread->thread_id) {
          LOG (debug) << "tv: got message updated.";
          // Note that the message has already been refreshed internally

          refptr<Message> _m = refptr<Message> (m);
          _m->reference (); // since m is owned by caller

          page_client->update_message (_m, AstroidMessages::UpdateMessage_Type_Tags);
          page_client->update_state ();
        }

      }
        else if (me ==
          Message::MessageChangedEvent::MESSAGE_REMOVED)
      {

        LOG (debug) << "tv: got message removed.";

        refptr<Message> _m = refptr<Message> (m);
        _m->reference (); // since m is owned by caller

        LOG (debug) << "tv: remove message: " << m->mid;
        state.erase (_m);

        /* check if message has been removed from messagethread, if not
         * message state will not be consistent between threads */
        for (auto &mm : mthread->messages) {
          if (m->mid == mm->mid) {
            LOG (error) << "tv: removed message, but it is still present in the MessageThread.";
            throw runtime_error ("tv: removed message, but it is still present in the MessageThread: inconsitent state.");
          }
        }

        page_client->remove_message (_m);
        page_client->update_state ();
      }
    }
  }
  /* end message loading  */

  /* rendering  */

  /* general message adding and rendering  */
  void ThreadView::load_html () {
    LOG (info) << "render: loading html..";
    wk_loaded = false;
    ready     = false;

    webkit_web_view_load_html (webview, theme.thread_view_html.c_str (), home_uri.c_str ());
  }

  void ThreadView::on_ready_to_render () {
    page_client->load ();

    /* render messages in case we were not ready when first requested */
    page_client->clear_messages ();
    render_messages ();
  }

  void ThreadView::render_messages () {
    LOG (debug) << "render: html loaded, building messages..";
    if (!wk_loaded) {
      LOG (error) << "tv: webkit not loaded.";
      return;
    }

    /* set message state vector */
    state.clear ();
    focused_message.clear ();

    if (mthread) {
      for (auto &m : mthread->messages) {
        add_message (m);
      }

      page_client->update_state ();
      update_all_indent_states ();

      /* focus oldest unread message */
      if (!edit_mode) {
        for (auto &m : mthread->messages_by_time ()) {
          if (m->has_tag ("unread")) {
            focused_message = m;
            break;
          }
        }
      }

      if (!focused_message) {
        LOG (debug) << "tv: no message focused, focusing newest message.";
        focused_message = *max_element (
            mthread->messages.begin (),
            mthread->messages.end (),
            [](refptr<Message> &a, refptr<Message> &b)
              {
                return ( a->time < b->time );
              });
      }

      expand (focused_message);
      focus_message (focused_message);

      ready = true;
      emit_ready ();

      if (!edit_mode && !unread_setup) {
        unread_setup = true;

        if (unread_delay > 0) {
          Glib::signal_timeout ().connect (
              sigc::mem_fun (this, &ThreadView::unread_check), std::max (80., (unread_delay * 1000.) / 2));
        } else {
          unread_check ();
        }
      }
    } else {
      LOG (debug) << "tv: no message thread.";
    }
  }

  void ThreadView::update_all_indent_states () {
    page_client->update_indent_state (indent_messages);
  }

  void ThreadView::add_message (refptr<Message> m) {
    LOG (debug) << "tv: adding message: " << m->mid;

    state.insert (std::pair<refptr<Message>, MessageState> (m, MessageState ()));

    m->signal_message_changed ().connect (
        sigc::mem_fun (this, &ThreadView::on_message_changed));

    page_client->add_message (m);

    if (!edit_mode) {
      /* optionally hide / collapse the message */
      if (!(m->has_tag("unread") || (expand_flagged && m->has_tag("flagged")))) {

        collapse (m);
      } else {
        expand (m);
        focused_message = m;
      }

    } else {
      /* edit mode */
      focused_message = m;
    }

    {
      if (!edit_mode &&
           any_of (Db::draft_tags.begin (),
                   Db::draft_tags.end (),
                   [&](ustring t) {
                     return m->has_tag (t);
                   }))
      {

        /* set warning */
        set_warning (m, "This message is a draft, edit it with E or delete with D.");

      }
    }
  }

  /* info and warning  */
  void ThreadView::set_warning (refptr<Message> m, ustring txt)
  {
    LOG (debug) << "tv: set warning: " << txt;
    page_client->set_warning (m, txt);
  }


  void ThreadView::hide_warning (refptr<Message> m)
  {
    page_client->hide_warning (m);
  }

  void ThreadView::set_info (refptr<Message> m, ustring txt)
  {
    LOG (debug) << "tv: set info: " << txt;
    page_client->set_info (m, txt);
  }

  void ThreadView::hide_info (refptr<Message> m) {
    page_client->hide_info (m);
  }
  /* end info and warning  */

  /* end rendering  */

  bool ThreadView::on_key_press_event (GdkEventKey * event) {
    if (!ready.load ()) return true;
    else return Mode::on_key_press_event (event);
  }

  void ThreadView::refresh () {
    LOG (debug) << "tv: reloading...";
    theme.load (true);

    Db db (Db::DbMode::DATABASE_READ_ONLY);
    auto _mthread = refptr<MessageThread>(new MessageThread (thread));
    _mthread->load_messages (&db);
    load_message_thread (_mthread);
  }

  void ThreadView::register_keys () { // {{{
    keys.title = "Thread View";

    keys.register_key ("$", "thread_view.reload",
        "Reload everything",
        [&] (Key) {
          refresh();
          return true;
        });

# ifdef DEBUG_WEBKIT
    keys.register_key ("C-I", "thread_view.show_web_inspector",
        "Show web inspector",
        [&] (Key) {
          LOG (debug) << "tv show web inspector";
          /* Show the inspector */
          WebKitWebInspector *inspector = webkit_web_view_get_inspector (WEBKIT_WEB_VIEW(webview));
          webkit_web_inspector_show (WEBKIT_WEB_INSPECTOR(inspector));
          return true;
        });

# endif

    keys.register_key ("j", "thread_view.down",
        "Scroll down or move focus to next element",
        [&] (Key) {
          page_client->focus_next_element (false);
          return true;
        });

    keys.register_key ("C-j", "thread_view.next_element",
        "Move focus to next element",
        [&] (Key) {
          /* move focus to next element and scroll to it if necessary */

          page_client->focus_next_element (true);
          return true;
        });

    keys.register_key ("J", { Key (GDK_KEY_Down) }, "thread_view.scroll_down",
        "Scroll down",
        [&] (Key) {
          page_client->scroll_down_big ();

          return true;
        });

    keys.register_key ("C-d", { Key (true, false, (guint) GDK_KEY_Down), Key (GDK_KEY_Page_Down) },
        "thread_view.page_down",
        "Page down",
        [&] (Key) {
          page_client->scroll_down_page ();
          return true;
        });

    keys.register_key ("k", "thread_view.up",
        "Scroll up or move focus to previous element",
        [&] (Key) {
          page_client->focus_previous_element (false);
          return true;
        });

    keys.register_key ("C-k", "thread_view.previous_element",
        "Move focus to previous element",
        [&] (Key) {
          page_client->focus_previous_element (true);
          return true;
        });

    keys.register_key ("K", { Key (GDK_KEY_Up) },
        "thread_view.scroll_up",
        "Scroll up",
        [&] (Key) {
          page_client->scroll_up_big ();
          return true;
        });

    keys.register_key ("C-u", { Key (true, false, (guint) GDK_KEY_Up), Key (GDK_KEY_Page_Up) },
        "thread_view.page_up",
        "Page up",
        [&] (Key) {
          page_client->scroll_up_page ();
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) },
        "thread_view.home",
        "Scroll home",
        [&] (Key) {
          page_client->scroll_to_top ();
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) },
        "thread_view.end",
        "Scroll to end",
        [&] (Key) {
          page_client->scroll_to_bottom ();
          return true;
        });

    keys.register_key (Key (GDK_KEY_Return), { Key (GDK_KEY_KP_Enter), Key (true, false, (guint) GDK_KEY_space) },
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

    keys.register_key ("d", "thread_view.delete_attachment",
        "Delete attachment (if editing)",
        [&] (Key) {
          if (edit_mode) {
            /* del attachment */
            return element_action (EDelete);
          }
          return false;
        });

    keys.register_key ("e", "thread_view.expand",
        "Toggle expand",
        [&] (Key) {
          if (edit_mode) return false;

          toggle (focused_message);

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
                        return !state[m].expanded;
                      }
                )) {
            /* all are hidden */
            for (auto m : mthread->messages) {
              expand (m);
            }

          } else {
            /* some are shown */
            for (auto m : mthread->messages) {
              collapse (m);
            }
          }

          return true;
        });

    keys.register_key ("t", "thread_view.mark",
        "Mark or unmark message",
        [&] (Key) {
          if (!edit_mode) {
            state[focused_message].marked = !(state[focused_message].marked);
            page_client->set_marked_state (focused_message, state[focused_message].marked);
            return true;
          }
          return false;
        });

    keys.register_key ("T", "thread_view.toggle_mark_all",
        "Toggle mark on all messages",
        [&] (Key) {
          if (!edit_mode) {

            bool any = false;
            bool all = true;

            for (auto &s : state) {
              if (s.second.marked) {
                any = true;
              } else {
                all = false;
              }

              if (any && !all) break;
            }

            for (auto &s : state) {
              if (any && !all) {
                s.second.marked = true;
              } else {
                s.second.marked = !s.second.marked;
              }
              page_client->set_marked_state (s.first, s.second.marked);
            }


            return true;
          }
          return false;
        });

    keys.register_key ("C-i", "thread_view.show_remote_images",
        "Show remote images (warning: approves all requests to remote content for this thread!)",
        [&] (Key) {
          /* we only allow remote images / resources when
           * no encrypted parts are present */
          if (!astroid->config("thread_view").get<bool> ("allow_remote_when_encrypted")) {
            for (auto &m : mthread->messages) {
              for (auto &c : m->all_parts()) {
                if (c->isencrypted) {
                  LOG (error) << "tv: remote resources are not allowed in encrypted messages. check your configuration if you wish to change this.";
                  return true;
                }
              }
            }
          }

          LOG (debug) << "tv: show remote images.";
          page_client->allow_remote_resources ();

          return true;
        });

    keys.register_key ("C-+", "thread_view.zoom_in",
        "Zoom in",
        [&] (Key) {
          webkit_web_view_set_zoom_level (webview,
              webkit_web_view_get_zoom_level (webview) + .1);

          return true;
        });

    keys.register_key ("C-minus", "thread_view.zoom_out",
        "Zoom out",
        [&] (Key) {
          webkit_web_view_set_zoom_level (webview,
              std::max ((webkit_web_view_get_zoom_level (webview) - .1), 0.0));

          return true;
        });

    keys.register_key ("S", "thread_view.save_all_attachments",
        "Save all attachments",
        [&] (Key) {
          save_all_attachments ();
          return true;
        });

    keys.register_key ("n", "thread_view.next_message",
        "Focus next message",
        [&] (Key) {
          page_client->focus_next_message ();
          return true;
        });

    keys.register_key ("C-n", "thread_view.next_message_expand",
        "Focus next message (and expand if necessary)",
        [&] (Key) {
          if (state[focused_message].scroll_expanded) {
            collapse (focused_message);
            state[focused_message].scroll_expanded = false;
          }

          page_client->focus_next_message ();

          state[focused_message].scroll_expanded = !expand (focused_message);
          return true;
        });

    keys.register_key ("p", "thread_view.previous_message",
        "Focus previous message",
        [&] (Key) {
          page_client->focus_previous_message (true);
          return true;
        });

    keys.register_key ("C-p", "thread_view.previous_message_expand",
        "Focus previous message (and expand if necessary)",
        [&] (Key) {
          if (state[focused_message].scroll_expanded) {
            collapse (focused_message);
            state[focused_message].scroll_expanded = false;
          }

          page_client->focus_previous_message (false);

          state[focused_message].scroll_expanded = !expand (focused_message);
          return true;
        });

    keys.register_key (Key (GDK_KEY_Tab), "thread_view.next_unread",
        "Focus the next unread message",
        [&] (Key) {
          bool foundme = false;

          for (auto &m : mthread->messages) {
            if (foundme && m->has_tag ("unread")) {
              focus_message (m);
              break;
            }

            if (m == focused_message) {
              foundme = true;
            }
          }

          return true;
        });

    keys.register_key (Key (false, false, (guint) GDK_KEY_ISO_Left_Tab),
        "thread_view.previous_unread",
        "Focus the previous unread message",
        [&] (Key) {
          bool foundme = false;

          for (auto mi = mthread->messages.rbegin ();
              mi != mthread->messages.rend (); mi++) {
            if (foundme && (*mi)->has_tag ("unread")) {
              focus_message (*mi);
              break;
            }

            if (*mi == focused_message) {
              foundme = true;
            }
          }

          return true;
        });

    keys.register_key ("c", "thread_view.compose_to_sender",
        "Compose a new message to the sender of the message (or all recipients if sender is you)",
        [&] (Key) {
          if (!edit_mode) {
            Address sender = focused_message->sender;
            Address from;
            AddressList to, cc, bcc;

            /* Send to original sender if message is not from own account,
               otherwise use all recipients as in the original */
            if (astroid->accounts->is_me (sender)) {
              from = sender;
              to   = AddressList(focused_message->to  ());
              cc   = AddressList(focused_message->cc  ());
              bcc  = AddressList(focused_message->bcc ());
            } else {
              /* Not from me, just use orginal sender */
              to += sender;

              /* find the 'first' me */
              AddressList tos = focused_message->all_to_from ();

              for (Address f : tos.addresses) {
                if (astroid->accounts->is_me (f)) {
                  from = f;
                  break;
                }
              }
            }
	    main_window->add_mode (new EditMessage (main_window, to.str (),
						    from.full_address (),
						    cc.str(), bcc.str()));
          }
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

    keys.register_key ("R", "thread_view.reply_sender",
        "Reply to sender of current message",
        [&] (Key) {
          /* reply to currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ReplyMessage (main_window, focused_message, ReplyMessage::ReplyMode::Rep_Sender));

            return true;
          }
          return false;
        });

    keys.register_key ("M", "thread_view.reply_mailinglist",
        "Reply to mailinglist of current message",
        [&] (Key) {
          /* reply to currently focused message */
          if (!edit_mode) {
            main_window->add_mode (new ReplyMessage (main_window, focused_message, ReplyMessage::ReplyMode::Rep_MailingList));

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


    keys.register_key ("C-F",
        "thread_view.flat",
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
                          return focused_message->has_tag (t);
                        }))
            {
              ask_yes_no ("Do you want to delete this draft? (any changes will be lost)",
                  [&](bool yes) {
                    if (yes) {
                      EditMessage::delete_draft (focused_message);
                      close ();
                    }
                  });

              return true;
            }
          }
          return false;
        });

    keys.register_run ("thread_view.run",
	[&] (Key, ustring cmd, ustring undo_cmd) {
          if (focused_message) {
            cmd = ustring::compose (cmd, focused_message->tid, focused_message->mid);
            undo_cmd = ustring::compose (undo_cmd, focused_message->tid, focused_message->mid);

            astroid->actions->doit (refptr<Action> (new CmdAction (
              Cmd ("thread_view.run", cmd, undo_cmd), focused_message->tid, focused_message->mid)));

            }
          return true;
        });

    multi_keys.register_key ("t", "thread_view.multi.toggle",
        "Toggle marked",
        [&] (Key) {
          for (auto &ms : state) {
            refptr<Message> m = ms.first;
            MessageState    s = ms.second;
            if (s.marked) {
              state[m].marked = false;
              page_client->set_marked_state (m, state[m].marked);
            }
          }
          return true;
        });

    multi_keys.register_key ("+", "thread_view.multi.tag",
        "Tag",
        [&] (Key) {
          /* TODO: Move this into a function in a similar way as multi_key_handler
           * for threadindex */


          /* ask for tags */
          main_window->enable_command (CommandBar::CommandMode::DiffTag,
              "",
              [&](ustring tgs) {
                LOG (debug) << "tv: got difftags: " << tgs;

                vector<refptr<NotmuchItem>> messages;

                for (auto &ms : state) {
                  refptr<Message> m = ms.first;
                  MessageState    s = ms.second;
                  if (s.marked) {
                    messages.push_back (m->nmmsg);
                  }
                }

                refptr<Action> ma = refptr<DiffTagAction> (DiffTagAction::create (messages, tgs));
                if (ma) {
                  main_window->actions->doit (ma);
                }
              });

          return true;
        });

    multi_keys.register_key ("C-y", "thread_view.multi.yank_mids",
        "Yank message id's",
        [&] (Key) {
          ustring ids = "";

          for (auto &m : mthread->messages) {
            MessageState s = state[m];
            if (s.marked) {
              ids += m->mid + ", ";
            }
          }

          ids = ids.substr (0, ids.length () - 2);

          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          cp->set_text (ids);

          LOG (info) << "tv: " << ids << " copied to clipboard.";

          return true;
        });

    multi_keys.register_key ("y", "thread_view.multi.yank",
        "Yank",
        [&] (Key) {
          ustring y = "";

          for (auto &m : mthread->messages) {
            MessageState s = state[m];
            if (s.marked) {
              y += m->plain_text (true);
              y += "\n";
            }
          }

          /* remove last newline */
          y = y.substr (0, y.size () - 1);

          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          cp->set_text (y);

          LOG (info) << "tv: yanked marked messages to clipobard.";

          return true;
        });

    multi_keys.register_key ("Y", "thread_view.multi.yank_raw",
        "Yank raw",
        [&] (Key) {
          /* tries to export the messages as an mbox file */
          ustring y = "";

          for (auto &m : mthread->messages) {
            MessageState s = state[m];
            if (s.marked) {
              auto d   = m->raw_contents ();
              auto cnv = UstringUtils::bytearray_to_ustring (d);
              if (cnv.first) {
                y += ustring::compose ("From %1  %2",
                    Address(m->sender).email(),
                    m->date_asctime ()); // asctime adds a \n

                y += UstringUtils::unixify (cnv.second);
                y += "\n";
              }
            }
          }

          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          cp->set_text (y);

          LOG (info) << "tv: yanked raw marked messages to clipobard.";

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
            LOG (debug) << "tv: saving messages: " << tosave.size();

            Gtk::FileChooserDialog dialog ("Save messages to folder..",
                Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

            dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
            dialog.add_button ("_Select", Gtk::RESPONSE_OK);
            dialog.set_current_folder (astroid->runtime_paths ().save_dir.c_str ());

            int result = dialog.run ();

            switch (result) {
              case (Gtk::RESPONSE_OK):
                {
                  string dir = dialog.get_filename ();
                  LOG (info) << "tv: saving messages to: " << dir;

                  astroid->runtime_paths ().save_dir = bfs::path (dialog.get_current_folder ());

                  for (refptr<Message> m : tosave) {
                    m->save_to (dir);
                  }

                  break;
                }

              default:
                {
                  LOG (debug) << "tv: save: cancelled.";
                }
            }
          }

          return true;
        });

    multi_keys.register_key ("p", "thread_view.multi.print",
        "Print marked messages",
        [&] (Key) {
          vector<refptr<Message>> toprint;
          for (auto &m : mthread->messages) {
            MessageState s = state[m];
            if (s.marked) {
              toprint.push_back (m);
            }
          }

# if 0
          GError * err = NULL;
          WebKitDOMDocument * d = webkit_web_view_get_dom_document (webview);
# endif

          for (auto &m : toprint) {
          // TODO: [JS] [REIMPLEMENT]
# if 0
            ustring mid = "message_" + m->mid;
            WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());
            WebKitDOMDOMTokenList * class_list =
              webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

            webkit_dom_dom_token_list_add (class_list, "print",
                (err = NULL, &err));
# endif

            /* expand */
            state[m].print_expanded = !expand (m);

          }

          bool indented = indent_messages;
          if (indent_messages) {
            indent_messages = false;
            update_all_indent_states ();
          }

          /* open print window */
          // TODO: [W2] Fix print
# if 0
          WebKitWebFrame * frame = webkit_web_view_get_main_frame (webview);
          webkit_web_frame_print (frame);
# endif

          if (indented) {
            indent_messages = true;
            update_all_indent_states ();
          }

          for (auto &m : toprint) {

            if (state[m].print_expanded) {
              collapse (m);
              state[m].print_expanded = false;
            }

# if 0
            ustring mid = "message_" + m->mid;
            WebKitDOMElement * e = webkit_dom_document_get_element_by_id (d, mid.c_str());
            WebKitDOMDOMTokenList * class_list =
              webkit_dom_element_get_class_list (WEBKIT_DOM_ELEMENT(e));

            webkit_dom_dom_token_list_remove (class_list, "print",
                (err = NULL, &err));

            g_object_unref (class_list);
            g_object_unref (e);
# endif
          }

# if 0
          g_object_unref (d);
# endif

          return true;
        });

    keys.register_key (Key (GDK_KEY_semicolon),
          "thread_view.multi",
          "Apply action to marked messages",
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

    keys.register_key ("N",
        "thread_view.toggle_unread",
        "Toggle the unread tag on the message",
        [&] (Key) {
          if (!edit_mode && focused_message) {

            main_window->actions->doit (refptr<Action>(new ToggleAction (refptr<NotmuchItem>(new NotmuchMessage(focused_message)), "unread")));
            state[focused_message].unread_checked = true;

          }

          return true;
        });

    keys.register_key ("*",
        "thread_view.flag",
        "Toggle the 'flagged' tag on the message",
        [&] (Key) {
          if (!edit_mode && focused_message) {

            main_window->actions->doit (refptr<Action>(new ToggleAction (refptr<NotmuchItem>(new NotmuchMessage(focused_message)), "flagged")));

          }

          return true;
        });

    keys.register_key ("a",
        "thread_view.archive_thread",
        "Toggle 'inbox' tag on the whole thread",
        [&] (Key) {

          if (!edit_mode && focused_message) {
            if (thread) {
              main_window->actions->doit (refptr<Action>(new ToggleAction(thread, "inbox")));
            }
          }

          return true;
        });

    keys.register_key ("#",
        "thread_view.trash_thread",
        "Toggle 'trash' tag on the whole thread",
        [&] (Key) {

          if (!edit_mode && focused_message) {
            if (thread) {
              main_window->actions->doit (refptr<Action>(new ToggleAction(thread, "trash")));
            }
          }

          return true;
        });

    keys.register_key ("C-P",
        "thread_view.print",
        "Print focused message",
        [&] (Key) {
        // TODO: [W2]
# if 0
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
# endif

          return true;
        });

    keys.register_key ("+",
        "thread_view.tag_message",
        "Tag message",
        [&] (Key) {
          if (edit_mode) {
            return false;
          }

          ustring tag_list = VectorUtils::concat_tags (focused_message->tags) + " ";

          main_window->enable_command (CommandBar::CommandMode::Tag,
              "Change tags for message:",
              tag_list,
              [&](ustring tgs) {
                LOG (debug) << "ti: got tags: " << tgs;

                vector<ustring> tags = VectorUtils::split_and_trim (tgs, ",| ");

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
                  LOG (debug) << "ti: nothing to do.";
                } else {
                  main_window->actions->doit (refptr<Action>(new TagAction (refptr<NotmuchItem>(new NotmuchMessage(focused_message)), add, rem)));
                }

                /* make sure that the unread tag is not modified after manually editing tags */
                state[focused_message].unread_checked = true;

              });
          return true;
        });

    keys.register_key ("C-f",
        "thread_view.search.search_or_next",
        "Search for text or go to next match",
        std::bind (&ThreadView::search, this, std::placeholders::_1, true));

    keys.register_key (UnboundKey (),
        "thread_view.search.search",
        "Search for text",
        std::bind (&ThreadView::search, this, std::placeholders::_1, false));


    keys.register_key (GDK_KEY_Escape, "thread_view.search.cancel",
        "Cancel current search",
        [&] (Key) {
          reset_search ();
          return true;
        });

    keys.register_key (UnboundKey (), "thread_view.search.next",
        "Go to next match",
        [&] (Key) {
          next_search_match ();
          return true;
        });

    keys.register_key ("P", "thread_view.search.previous",
        "Go to previous match",
        [&] (Key) {
          prev_search_match ();
          return true;
        });

    keys.register_key ("y", "thread_view.yank",
        "Yank current element or message text to clipboard",
        [&] (Key) {
          element_action (EYank);
          return true;
        });

    keys.register_key ("Y", "thread_view.yank_raw",
        "Yank raw content of current element or message to clipboard",
        [&] (Key) {
          element_action (EYankRaw);
          return true;
        });

    keys.register_key ("C-y", "thread_view.yank_mid",
        "Yank the Message-ID of the focused message to clipboard",
        [&] (Key) {
          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          cp->set_text (focused_message->mid);

          LOG (info) << "tv: " << focused_message->mid << " copied to clipboard.";

          return true;
        });

    keys.register_key (Key (":"),
          "thread_view.multi_next_thread",
          "Open next after..",
          [&] (Key k) {
            multi_key (next_multi, k);

            return true;
          });

    next_multi.title = "Thread";
    next_multi.register_key (Key ("a"),
        "thread_view.multi_next_thread.archive",
        "Archive, goto next",
        [&] (Key) {
          keys.handle ("thread_view.archive_thread");
          emit_index_action (IA_Next);

          return true;
        });

    next_multi.register_key (Key ("A"),
        "thread_view.multi_next_thread.archive_next_unread_thread",
        "Archive, goto next unread",
        [&] (Key) {
          keys.handle ("thread_view.archive_thread");
          emit_index_action (IA_NextUnread);

          return true;
        });

    next_multi.register_key ("#",
        "thread_view.multi_next_thread.trash_thread",
        "Trash, goto next",
        [&] (Key) {
          keys.handle ("thread_view.trash_thread");
          emit_index_action (IA_Next);

          return true;
        });

    next_multi.register_key (Key ("x"),
        "thread_view.multi_next_thread.close",
        "Archive, close",
        [&] (Key) {
          keys.handle ("thread_view.archive_thread");
          close ();

          return true;
        });

    next_multi.register_key (Key ("j"),
        "thread_view.multi_next_thread.next_thread",
        "Goto next",
        [&] (Key) {
          emit_index_action (IA_Next);

          return true;
        });

    next_multi.register_key (Key ("k"),
        "thread_view.multi_next_thread.previous_thread",
        "Goto previous",
        [&] (Key) {
          emit_index_action (IA_Previous);

          return true;
        });

    next_multi.register_key (Key (GDK_KEY_Tab),
        "thread_view.multi_next_thread.next_unread",
        "Goto next unread",
        [&] (Key) {
          emit_index_action (IA_NextUnread);

          return true;
        });

    next_multi.register_key (Key (false, false, (guint) GDK_KEY_ISO_Left_Tab),
        "thread_view.multi_next_thread.previous_unread",
        "Goto previous unread",
        [&] (Key) {
          emit_index_action (IA_PreviousUnread);

          return true;
        });

    /* make aliases in main namespace */
    keys.register_key (UnboundKey (),
        "thread_view.archive_then_next",
        "Alias for thread_view.multi_next_thread.archive",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.archive");
        });

    keys.register_key (UnboundKey (),
        "thread_view.archive_then_next_unread",
        "Alias for thread_view.multi_next_thread.archive_next_unread",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.archive_next_unread_thread");
        });

    keys.register_key (UnboundKey (),
        "thread_view.archive_and_close",
        "Alias for thread_view.multi_next_thread.close",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.close");
        });

    keys.register_key (UnboundKey (),
        "thread_view.next_thread",
        "Alias for thread_view.multi_next_thread.next_thread",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.next_thread");
        });

    keys.register_key (UnboundKey (),
        "thread_view.previous_thread",
        "Alias for thread_view.multi_next_thread.previous_thread",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.previous_thread");
        });

    keys.register_key (UnboundKey (),
        "thread_view.next_unread_thread",
        "Alias for thread_view.multi_next_thread.next_unread",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.next_unread");
        });

    keys.register_key (UnboundKey (),
        "thread_view.previous_unread_thread",
        "Alias for thread_view.multi_next_thread.previous_unread",
        [&] (Key) {
          return next_multi.handle ("thread_view.multi_next_thread.previous_unread");
        });


  } // }}}

  bool ThreadView::element_action (ElementAction a) { // {{{
    LOG (debug) << "tv: activate item.";

    if (!(focused_message)) {
      LOG (error) << "tv: no message has focus yet.";
      return true;
    }

    if (!edit_mode && !state[focused_message].expanded) {
      if (a == EEnter) {
        toggle (focused_message);
      } else if (a == ESave) {
        /* save message to */
        focused_message->save ();

      } else if (a == EYankRaw) {
        auto    cp = Gtk::Clipboard::get (astroid->clipboard_target);
        ustring t  = "";

        auto d   = focused_message->raw_contents ();
        auto cnv = UstringUtils::bytearray_to_ustring (d);
        if (cnv.first) {
          t = UstringUtils::unixify (cnv.second);
        }

        cp->set_text (t);

      } else if (a == EYank) {

        auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
        ustring t;

        t = focused_message->plain_text (true);

        cp->set_text (t);
      }

    } else {
      if (state[focused_message].current_element == 0) {
        if (!edit_mode && a == EEnter) {
          /* nothing selected, closing message */
          toggle (focused_message);
        } else if (a == ESave) {
          /* save message to */
          focused_message->save ();
        } else if (a == EYankRaw) {
          auto    cp = Gtk::Clipboard::get (astroid->clipboard_target);
          ustring t  = "";

          auto d = focused_message->raw_contents ();
          auto cnv = UstringUtils::bytearray_to_ustring (d);
          if (cnv.first) {
            t = UstringUtils::unixify (cnv.second);
          }

          cp->set_text (t);

        } else if (a == EYank) {

          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          ustring t;

          t = focused_message->plain_text (true);

          cp->set_text (t);
        }

      } else {
        if (a == EYankRaw) {

          refptr<Chunk> c = focused_message->get_chunk_by_id (
              state[focused_message].elements[state[focused_message].current_element].id);
          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          ustring t = "";

          auto d   = c->contents ();
          auto cnv = UstringUtils::bytearray_to_ustring (d);
          if (cnv.first) {
            t = cnv.second;
          }

          cp->set_text (t);

        } else if (a == EYank) {

          refptr<Chunk> c = focused_message->get_chunk_by_id (
              state[focused_message].elements[state[focused_message].current_element].id);
          auto cp = Gtk::Clipboard::get (astroid->clipboard_target);
          ustring t;

          if (c->viewable) {
            t = c->viewable_text (false, false);
          } else {
            LOG (error) << "tv: cannot yank text of non-viewable part";
          }

          cp->set_text (t);

        } else {

          switch (state[focused_message].elements[state[focused_message].current_element].type) {
            case MessageState::ElementType::Attachment:
              {
                if (a == EEnter) {
                  /* open attachment */

                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->open ();
                  } else {
                    LOG (error) << "tv: could not find chunk for element.";
                  }

                } else if (a == ESave) {
                  /* save attachment */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->save ();
                  } else {
                    LOG (error) << "tv: could not find chunk for element.";
                  }
                }
              }
              break;
            case MessageState::ElementType::Part:
              {
                if (a == EEnter) {
                  /* open part */

                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    if (c->get_content_type() != "text/plain" && open_html_part_external) {
                      c->open ();
                    } else {
                      /* show part internally */

                      page_client->toggle_part (focused_message, c, state[focused_message].elements[state[focused_message].current_element]);

                    }
                  } else {
                    LOG (error) << "tv: could not find chunk for element.";
                  }

                } else if (a == ESave) {
                  /* save part */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->save ();
                  } else {
                    LOG (error) << "tv: could not find chunk for element.";
                  }
                }

              }
              break;

            case MessageState::ElementType::MimeMessage:
              {
                if (a == EEnter) {
                  /* open part */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  refptr<MessageThread> mt = refptr<MessageThread> (new MessageThread ());
                  mt->add_message (c);

                  ThreadView * tv = Gtk::manage(new ThreadView (main_window));
                  tv->load_message_thread (mt);

                  main_window->add_mode (tv);

                } else if (a == ESave) {
                  /* save part */
                  refptr<Chunk> c = focused_message->get_chunk_by_id (
                      state[focused_message].elements[state[focused_message].current_element].id);

                  if (c) {
                    c->save ();
                  } else {
                    LOG (error) << "tv: could not find chunk for element.";
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

  bool ThreadView::unread_check () {
    if (!ready) {
      unread_setup = false;
      return false; // disconnect
    }

    if (!edit_mode && focused_message && focused_message->in_notmuch) {

      if (!state[focused_message].unread_checked && state[focused_message].expanded) {

        chrono::duration<double> elapsed = chrono::steady_clock::now() - focus_time;

        if (unread_delay == 0.0 || elapsed.count () > unread_delay) {
          if (focused_message->has_tag ("unread")) {

            main_window->actions->doit (refptr<Action>(new TagAction (refptr<NotmuchItem>(new NotmuchMessage(focused_message)), {}, { "unread" })), false);
            state[focused_message].unread_checked = true;
          }
        }
      }
    }

    return true;
  }

  void ThreadView::focus_element (refptr<Message> m, unsigned int e) {
    if (m) {
      LOG (debug) << "tv: focus message: " << m->safe_mid () << ", element: " << e;

      page_client->focus_element (m, e);
    }
  }

  void ThreadView::focus_message (refptr<Message> m) {
    focus_element (m, 0);
  }
  /* end focus handeling   */

  /* message expanding and collapsing  */
  bool ThreadView::expand (refptr<Message> m) {
    /* returns true if the message was expanded in the first place */
    bool wasexpanded  = state[m].expanded;

    state[m].expanded = true;
    page_client->set_hidden_state (m, false);

    if (!wasexpanded) {
      /* if the message was unexpanded, it would not have been marked as read */
      if (unread_delay == 0.0) unread_check ();
    }

    return wasexpanded;
  }

  bool ThreadView::collapse (refptr<Message> m) {
    /* returns true if the message was expanded in the first place */
    bool wasexpanded  = state[m].expanded;

    page_client->set_hidden_state (m, true);
    state[m].expanded = false;


    return wasexpanded;
  }

  bool ThreadView::toggle (refptr<Message> m)
  {
    /* returns true if the message was expanded in the first place */
    if (state[m].expanded) return collapse (m);
    else                   return expand (m);
  }

  /* end message expanding and collapsing  */

  void ThreadView::save_all_attachments () { //
    /* save all attachments of current focused message */
    LOG (info) << "tv: save all attachments..";

    if (!focused_message) {
      LOG (warn) << "tv: no message focused!";
      return;
    }

    auto attachments = focused_message->attachments ();
    if (attachments.empty ()) {
      LOG (warn) << "tv: this message has no attachments to save.";
      return;
    }

    Gtk::FileChooserDialog dialog ("Save attachments to folder..",
        Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

    dialog.add_button ("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button ("_Select", Gtk::RESPONSE_OK);
    dialog.set_current_folder (astroid->runtime_paths ().save_dir.c_str ());

    int result = dialog.run ();

    switch (result) {
      case (Gtk::RESPONSE_OK):
        {
          string dir = dialog.get_filename ();
          LOG (info) << "tv: saving attachments to: " << dir;

          astroid->runtime_paths ().save_dir = bfs::path (dialog.get_current_folder ());

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
          LOG (debug) << "tv: save: cancelled.";
        }
    }
  } //

  /* general mode stuff  */
  void ThreadView::grab_focus () {
    //LOG (debug) << "tv: grab focus";
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

  /* end general mode stuff  */

  /* signals  */
  ThreadView::type_signal_ready
    ThreadView::signal_ready ()
  {
    return m_signal_ready;
  }

  void ThreadView::emit_ready () {
    LOG (info) << "tv: ready emitted.";
    ready = true;
    m_signal_ready.emit ();
  }

  ThreadView::type_element_action
    ThreadView::signal_element_action ()
  {
    return m_element_action;
  }

  void ThreadView::emit_element_action (unsigned int element, ElementAction action) {
    LOG (debug) << "tv: element action emitted: " << element << ", action: enter";
    m_element_action.emit (element, action);
  }

  ThreadView::type_index_action ThreadView::signal_index_action () {
    return m_index_action;
  }

  bool ThreadView::emit_index_action (IndexAction action) {
    LOG (debug) << "tv: index action: " << action;
    return m_index_action.emit (this, action);
  }

  /* end signals  */

  /* MessageState   */
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

  ThreadView::MessageState::Element * ThreadView::MessageState::get_element_by_id (int id) {
    for (auto e : elements) {
      LOG (debug) << "e: " << e.id;
    }

    auto e = std::find_if (
        elements.begin (), elements
        .end (),
        [&] (auto e) { return e.id == id; } );

    if (e == elements.end ()) {
      LOG (error) << "tv: e == NULL";
    }

    return (e != elements.end() ? &(*e) : NULL);
  }

  /* end MessageState  */

  /* Searching  */
  bool ThreadView::search (Key, bool next) {
    if (in_search && next) {
      next_search_match ();
      return true;
    }

    reset_search ();

    main_window->enable_command (CommandBar::CommandMode::SearchText,
        "", sigc::mem_fun (this, &ThreadView::on_search));

    return true;
  }

  void ThreadView::reset_search () {
    /* reset */
    if (in_search) {
      /* reset search expanded state */
      for (auto m : mthread->messages) {
        state[m].search_expanded = false;
      }
    }

    in_search = false;
    search_q  = "";

    WebKitFindController * f = webkit_web_view_get_find_controller (webview);
    webkit_find_controller_search_finish (f);
  }

  void ThreadView::on_search (ustring k) {
    if (!k.empty ()) {

      /* expand all messages, these should be closed - except the focused one
       * when a search is cancelled */
      for (auto m : mthread->messages) {
        state[m].search_expanded = !expand (m);
      }

      LOG (debug) << "tv: searching for: " << k;
      WebKitFindController * f = webkit_web_view_get_find_controller (webview);

      webkit_find_controller_search (f, k.c_str (),
          WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
          WEBKIT_FIND_OPTIONS_WRAP_AROUND,
          0);
      in_search = true;
      page_client->update_focus_to_view ();
    }
  }

  void ThreadView::next_search_match () {
    if (!in_search) return;
    /* there does not seem to be a way to figure out which message currently
     * contains the selected matched text, but when there is a scroll event
     * the match is centered.
     */

    WebKitFindController * f = webkit_web_view_get_find_controller (webview);
    webkit_find_controller_search_next (f);
    page_client->update_focus_to_view ();
  }

  void ThreadView::prev_search_match () {
    if (!in_search) return;

    WebKitFindController * f = webkit_web_view_get_find_controller (webview);
    webkit_find_controller_search_previous (f);
    page_client->update_focus_to_view ();
  }

  /***************
   * Exceptions
   ***************/

  webkit_error::webkit_error (const char * w) : std::runtime_error (w)
  {
  }
}

