# pragma once

# include <atomic>
# include <map>
# include <vector>
# include <string>
# include <chrono>
# include <mutex>
# include <condition_variable>
# include <functional>

# include <gtkmm.h>
# include <webkit2/webkit2.h>
# include <webkitdom/webkitdom.h>
# include <boost/property_tree/ptree.hpp>

# include "proto.hh"
# include "modes/mode.hh"
# include "message_thread.hh"
# include "theme.hh"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

using boost::property_tree::ptree;

namespace Astroid {
  extern "C" bool ThreadView_on_load_changed (
      WebKitWebView * w,
      WebKitLoadEvent load_event,
      gpointer user_data);

  extern "C" gboolean ThreadView_decide_policy (
      WebKitWebView * w,
      WebKitPolicyDecision * decision,
      WebKitPolicyDecisionType decision_type,
      gpointer user_data);

  class ThreadView : public Mode {
    friend PageClient;
    friend EditMessage;

    public:
      ThreadView (MainWindow *, bool _edit_mode = false);
      ~ThreadView ();
      void load_thread (refptr<NotmuchThread>);
      void load_message_thread (refptr<MessageThread>);

      refptr<NotmuchThread> thread; // will be refreshed through mthread
      refptr<MessageThread> mthread;

      bool      indent_messages;

    protected:
      bool edit_mode = false;

      double unread_delay = .5;
      std::chrono::time_point<std::chrono::steady_clock> focus_time;
      bool unread_check ();
      bool unread_setup = false;
      sigc::connection unread_checker;

      /* resources */
      ustring home_uri;           // relative url for requests

      bool    expand_flagged;

      Theme theme;

# ifndef DISABLE_PLUGINS
      PluginManager::ThreadViewExtension * plugins;
# endif

      ustring open_external_link;
      void    open_link (ustring);
      void    do_open_link (ustring);

      void pre_close () override;

      /* Web extension */
      PageClient * page_client;

    private:
      /* focus and message state */
      void focus_message (refptr<Message>);
      void focus_element (refptr<Message>, unsigned int);

      bool expand   (refptr<Message>);
      bool collapse (refptr<Message>);
      bool toggle   (refptr<Message>);

    public:
      /* message display state
       *
       * IMPORTANT: This should match the structure in messages.proto
       *
       */
      struct MessageState {
        public:
          MessageState ();

          bool expanded         = true;

          /* the message was expanded as part of an
           * C-n or C-p command */
          bool search_expanded  = false;
          bool scroll_expanded  = false;
          bool print_expanded   = false;
          bool marked           = false;
          bool unread_checked   = false;

          enum ElementType {
            Empty = 0,
            Address,
            Part,
            Attachment,
            MimeMessage,
            Encryption,
          };

          struct Element {
            public:
              Element (ElementType t, int i);
              ElementType type;
              int         id;
              ustring     element_id ();
              bool        focusable = true;

              bool operator== ( const Element & other ) const;
          };

          /* ordered list of elements, must be listed in order of
           * appearance.
           *
           * first element is always empty and represents the message
           * itself.
           */
          std::vector<Element> elements;
          unsigned int    current_element;
          Element * get_current_element ();
          Element * get_element_by_id (int id);
      };

      std::map<refptr<Message>, MessageState> state;

      refptr<Message> focused_message;

      /* set the warning header of the message */
      void set_warning (refptr<Message>, ustring);
      void hide_warning (refptr<Message>);

      /* set the info header of the message */
      void set_info (refptr<Message>, ustring);
      void hide_info (refptr<Message>);

      /* activate message or selected element */
      typedef enum {
        EEnter = 0,
        ESave,
        EDelete,
        EYankRaw,
        EYank,
      } ElementAction;

      bool element_action (ElementAction);

      /* webkit */
      WebKitWebView *     webview;
      WebKitSettings *    websettings;
      WebKitWebContext *  context;

    protected:
      std::atomic<bool> wk_loaded;

      /* rendering */
      void on_ready_to_render ();
      void load_html ();
      void render_messages ();

      /* message loading and rendering */
      void add_message (refptr<Message>);

      bool open_html_part_external;

      void update_all_indent_states ();

      void save_all_attachments ();
    public:

      /* event wrappers */
      bool on_load_changed (
        WebKitWebView * w,
        WebKitLoadEvent load_event);

      gboolean decide_policy (
          WebKitWebView * w,
          WebKitPolicyDecision * decision,
          WebKitPolicyDecisionType decision_type);

      void grab_focus ();

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
      bool on_key_press_event (GdkEventKey *event) override;

    private:
      Keybindings multi_keys;
      Keybindings next_multi;
      void register_keys ();

      /* changed signals */
      void on_message_changed (Db *, Message *, Message::MessageChangedEvent);

      /* search */
      bool search (Key, bool);
      void on_search (ustring);
      void reset_search ();

      void next_search_match ();
      void prev_search_match ();

      bool in_search = false;
      ustring search_q = "";

    public:
      /* the tv is ready */
      typedef sigc::signal <void> type_signal_ready;
      type_signal_ready signal_ready ();

      void emit_ready ();
      std::atomic<bool> ready; // all messages and elements rendered

      /* action on element */
      typedef sigc::signal <void, unsigned int, ElementAction> type_element_action;
      type_element_action signal_element_action ();

      void emit_element_action (unsigned int element, ElementAction action);

      /* actions for originating thread-index */
      typedef enum {
        IA_Next = 0,
        IA_Previous,
        IA_NextUnread,
        IA_PreviousUnread,
      } IndexAction;

      typedef sigc::signal <bool, ThreadView *, IndexAction> type_index_action;
      type_index_action signal_index_action ();

      bool emit_index_action (IndexAction action);

    protected:
      type_signal_ready m_signal_ready;
      type_element_action m_element_action;
      type_index_action m_index_action;
  };

  /* exceptions */
  class webkit_error : public std::runtime_error {
    public:
      webkit_error (const char *);

  };
}

