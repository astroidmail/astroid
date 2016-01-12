# pragma once

# include <iostream>
# include <atomic>
# include <map>

# include <gtkmm.h>
# include <webkit/webkit.h>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "proto.hh"
# include "modes/mode.hh"
# include "message_thread.hh"

# include "web_inspector.hh"

using namespace std;
using namespace boost::filesystem;

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
    public:
      ThreadView (MainWindow *);
      ~ThreadView ();
      void load_thread (refptr<NotmuchThread>);
      void load_message_thread (refptr<MessageThread>);

      refptr<NotmuchThread> thread;
      refptr<MessageThread> mthread;

      Gtk::ScrolledWindow scroll;

      const int MAX_PREVIEW_LEN = 80;

      const int INDENT_PX       = 20;
      bool      indent_messages;

      bool edit_mode = false;
      bool show_remote_images = false;

      /* resources */
      bool    enable_mathjax;
      ustring mathjax_uri_prefix;
      vector<ustring> mathjax_only_tags;
      ustring home_uri;           // relative url for requests
      bool    math_is_on = false; // for this thread

      bool    enable_code_prettify;
      vector<ustring> code_prettify_only_tags;
      ustring code_prettify_prefix; // add run_prettify.js to this
      ustring code_prettify_code_tag;
      bool    enable_code_prettify_for_patches;
      ustring code_start_tag = "<code class=\"prettyprint\">";
      ustring code_stop_tag  = "</code>";

      bool    code_is_on = false; // for this thread
      void    filter_code_tags (ustring &); // look for code tags


    private:
      /* message manipulation and location */
      void scroll_to_message (refptr<Message>, bool = false);
      bool scroll_to_element (ustring, bool = false);

      bool in_scroll = false;
      ustring         scroll_arg;
      bool            _scroll_when_visible;

      void update_focus_to_view ();
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

      void toggle_hidden (refptr<Message> = refptr<Message> (), ToggleState = ToggleToggle);
      bool is_hidden (refptr<Message>);

      /* message display state */
      struct MessageState {
        public:
          MessageState ();

          /* the message was expanded as part of an
           * C-n or C-p command */
          bool scroll_expanded = false;
          bool marked = false;

          enum ElementType {
            Empty,
            Address,
            Part,
            Attachment,
            MimeMessage
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
          vector<Element> elements;
          unsigned int    current_element;
          Element * get_current_element ();
      };

      std::map<refptr<Message>, MessageState> state;

      /* focused message */
      refptr<Message> candidate_startup; // startup

    public:
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
      } ElementAction;

      bool element_action (ElementAction);

    private:
      /* webkit (using C api) */
      WebKitWebView     * webview;
      WebKitWebSettings * websettings;
      WebKitDOMHTMLDivElement * container = NULL;
      WebKitWebInspector * web_inspector;

      static bool theme_loaded;
      static const char *  thread_view_html_f;
      static const char *  thread_view_css_f;
      static ustring       thread_view_html;
      static ustring       thread_view_css;
      const char * STYLE_NAME = "STYLE";
      const int THEME_VERSION = 1;
      bool check_theme_version (path);

      atomic<bool> wk_loaded;

      /* rendering */
      void render ();
      void render_messages ();
      void add_message (refptr<Message>);
      void reload_images ();
      WebKitDOMHTMLElement * make_message_div ();

      /* message loading */
      void set_message_html (refptr<Message>, WebKitDOMHTMLElement *);
      void create_message_part_html (refptr<Message>, refptr<Chunk>, WebKitDOMHTMLElement *, bool);
      void create_sibling_part (refptr<Message>, refptr<Chunk>, WebKitDOMHTMLElement *);
      void create_body_part (refptr<Message>, refptr<Chunk>, WebKitDOMHTMLElement *);
      void insert_header_address (ustring &, ustring, Address, bool);
      void insert_header_address_list (ustring &, ustring, AddressList, bool);
      void insert_header_row (ustring &, ustring, ustring, bool);
      void insert_header_date (ustring &, refptr<Message>);
      ustring create_header_row (ustring, ustring, bool, bool);

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

      /* webkit dom utils */
      WebKitDOMHTMLElement * clone_select (
          WebKitDOMNode * node,
          ustring         selector,
          bool            deep = true);

      WebKitDOMHTMLElement * clone_node (
          WebKitDOMNode * node,
          bool            deep = true);

      WebKitDOMHTMLElement * select (
          WebKitDOMNode * node,
          ustring         selector);

    public:
      static string assemble_data_uri (ustring, gchar *&, gsize);

      /* event wrappers */
      bool on_load_changed (
          GtkWidget *,
          GParamSpec *);

      void on_scroll_vadjustment_changed();

      Gtk::Window * inspector_window;
      Gtk::ScrolledWindow inspector_scroll;
      WebKitWebView * activate_inspector (
          WebKitWebInspector *,
          WebKitWebView *);
      bool show_inspector (WebKitWebInspector *);

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

    private:
      Keybindings multi_keys;
      void register_keys ();

    public:
      /* the tv is ready */
      typedef sigc::signal <void> type_signal_ready;
      type_signal_ready signal_ready ();

      void emit_ready ();
      atomic<bool> ready; // all messages and elements rendered

      /* action on element */
      typedef sigc::signal <void, unsigned int, ElementAction> type_element_action;
      type_element_action signal_element_action ();

      void emit_element_action (unsigned int element, ElementAction action);

    protected:
      type_signal_ready m_signal_ready;
      type_element_action m_element_action;
  };
}

