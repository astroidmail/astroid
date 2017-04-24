# pragma once

# include <atomic>
# include <map>
# include <vector>
# include <string>
# include <chrono>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "proto.hh"
# include "modes/mode.hh"
# include "message_thread.hh"
# include "theme.hh"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

# include "web_inspector.hh"

namespace Astroid {
  extern "C" bool ThreadView_on_load_changed (
          GtkWidget *,
          GParamSpec *,
          gpointer );

  extern "C" gboolean ThreadView_navigation_request (
      WebKitWebView * w,
      WebKitWebFrame * frame,
      WebKitNetworkRequest * request,
      WebKitWebNavigationAction * navigation_action,
      WebKitWebPolicyDecision * policy_decision,
      gpointer user_data);

  extern "C" void ThreadView_resource_request_starting (
      WebKitWebView         *web_view,
      WebKitWebFrame        *web_frame,
      WebKitWebResource     *web_resource,
      WebKitNetworkRequest  *request,
      WebKitNetworkResponse *response,
      gpointer               user_data);

  class ThreadView : public Mode {
    friend class ThreadViewInspector;

    public:
      ThreadView (MainWindow *);
      ~ThreadView ();
      void load_thread (refptr<NotmuchThread>);
      void load_message_thread (refptr<MessageThread>);

      refptr<NotmuchThread> thread; // will be refreshed through mthread
      refptr<MessageThread> mthread;

      Gtk::ScrolledWindow scroll;

      const int MAX_PREVIEW_LEN = 80;

      const int INDENT_PX       = 20;
      bool      indent_messages;

      bool edit_mode = false;
      bool show_remote_images = false;

      double unread_delay = .5;
      std::chrono::time_point<std::chrono::steady_clock> focus_time;
      bool unread_check ();
      bool unread_setup = false;

      /* resources */
      ustring home_uri;           // relative url for requests

      bool    expand_flagged;
      bool    enable_code_prettify;
      std::vector<ustring> code_prettify_only_tags;
      ustring code_prettify_uri = "https://cdn.rawgit.com/google/code-prettify/master/loader/run_prettify.js";
      ustring code_prettify_code_tag;
      bool    enable_code_prettify_for_patches;
      ustring code_start_tag = "<code class=\"prettyprint\">";
      ustring code_stop_tag  = "</code>";

      bool    code_is_on = false; // for this thread
      void    filter_code_tags (ustring &); // look for code tags

      bool enable_gravatar;

      Theme theme;

# ifndef DISABLE_PLUGINS
      PluginManager::ThreadViewExtension * plugins;
# endif

      void pre_close () override;

    private:
      ThreadViewInspector thread_view_inspector;

      /* message manipulation and location */
      bool scroll_to_message (refptr<Message>, bool = false);
      bool scroll_to_element (ustring, bool = false);

      bool    in_scroll = false;
      ustring scroll_arg;
      bool    _scroll_when_visible;

      void update_focus_to_view ();
      void update_focus_to_center ();
      void update_focus_status ();
      ustring focus_next ();
      ustring focus_previous (bool focus_top = false);

      ustring focus_next_element (bool = false);
      ustring focus_previous_element (bool = false);

      enum ToggleState {
        ToggleToggle,
        ToggleHide,
        ToggleShow
      };

      bool toggle_hidden (refptr<Message> = refptr<Message> (), ToggleState = ToggleToggle);
      bool is_hidden (refptr<Message>);

      /* focused message */
      refptr<Message> candidate_startup; // startup

    public:
      /* message display state */
      struct MessageState {
        public:
          MessageState ();

          /* the message was expanded as part of an
           * C-n or C-p command */
          bool search_expanded  = false;
          bool scroll_expanded  = false;
          bool print_expanded   = false;
          bool marked           = false;
          bool unread_checked   = false;

          enum ElementType {
            Empty,
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
        EOpen,
        ESave,
        EDelete,
        EYankRaw,
        EYank,
      } ElementAction;

      bool element_action (ElementAction);

      /* webkit (using C api) */
      WebKitWebView     * webview;
      WebKitWebSettings * websettings;

    private:
      WebKitDOMHTMLDivElement * container = NULL;

      std::atomic<bool> wk_loaded;

      /* rendering */
      void render ();
      void render_messages ();
      void add_message (refptr<Message>);
      void reload_images ();

      /* message loading */
      void set_message_html (refptr<Message>, WebKitDOMHTMLElement *);
      void create_message_part_html (refptr<Message>, refptr<Chunk>, WebKitDOMHTMLElement *, bool);
      void create_sibling_part (refptr<Message>, refptr<Chunk>, WebKitDOMHTMLElement *);
      void create_body_part (refptr<Message>, refptr<Chunk>, WebKitDOMHTMLElement *);
      void insert_header_address (ustring &, ustring, Address, bool);
      void insert_header_address_list (ustring &, ustring, AddressList, bool);
      void insert_header_row (ustring &, ustring, ustring, bool);
      void insert_header_date (ustring &, refptr<Message>);
      ustring create_header_row (ustring title, ustring value, bool important, bool escape, bool noprint = false);
      ustring header_row_value (ustring value, bool escape);
      void message_render_tags (refptr<Message>, WebKitDOMElement * div_message);
      void message_update_css_tags (refptr<Message>, WebKitDOMElement * div_message);

      bool open_html_part_external;
      void display_part (refptr<Message>, refptr<Chunk>, MessageState::Element);

      void update_all_indent_states ();
      void update_indent_state (refptr<Message>);

      /* marked */
      refptr<Gdk::Pixbuf> marked_icon;
      void load_marked_icon (refptr<Message>, WebKitDOMHTMLElement *);
      void update_marked_state (refptr<Message>);

      /* attachments */
      bool insert_attachments (refptr<Message>, WebKitDOMHTMLElement *);
      void set_attachment_icon (refptr<Message>, WebKitDOMHTMLElement *);

      /* mime messages */
      void insert_mime_messages (refptr<Message>, WebKitDOMHTMLElement *);

      void set_attachment_src (refptr<Chunk>,
          refptr<Glib::ByteArray>,
          WebKitDOMHTMLImageElement *);

      refptr<Gdk::Pixbuf> attachment_icon;

      static const int THUMBNAIL_WIDTH = 150; // px
      static const int ATTACHMENT_ICON_WIDTH = 35;

      void save_all_attachments ();
    public:

      /* event wrappers */
      bool on_load_changed (
          GtkWidget *,
          GParamSpec *);

      void on_scroll_vadjustment_changed();


      gboolean navigation_request (
        WebKitWebView * w,
        WebKitWebFrame * frame,
        WebKitNetworkRequest * request,
        WebKitWebNavigationAction * navigation_action,
        WebKitWebPolicyDecision * policy_decision);

      ustring open_external_link;
      void open_link (ustring);
      void do_open_link (ustring);

      void resource_request_starting (
        WebKitWebView         *web_view,
        WebKitWebFrame        *web_frame,
        WebKitWebResource     *web_resource,
        WebKitNetworkRequest  *request,
        WebKitNetworkResponse *response);


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
      bool in_search_match = false;
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
}

