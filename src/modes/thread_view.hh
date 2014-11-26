# pragma once

# include <iostream>
# include <atomic>
# include <map>

# include <gtkmm.h>
# include <webkit/webkit.h>

# include "astroid.hh"
# include "proto.hh"
# include "mode.hh"
# include "message_thread.hh"

using namespace std;

namespace Astroid {
  extern "C" bool ThreadView_on_load_changed (
          GtkWidget *,
          GParamSpec *,
          gpointer );

  extern "C" WebKitWebView * ThreadView_activate_inspector (
      WebKitWebInspector *,
      WebKitWebView *,
      gpointer );

  extern "C" bool ThreadView_show_inspector (
      WebKitWebInspector *,
      gpointer);

  extern "C" gboolean ThreadView_navigation_request (
      WebKitWebView * w,
      WebKitWebFrame * frame,
      WebKitNetworkRequest * request,
      WebKitWebNavigationAction * navigation_action,
      WebKitWebPolicyDecision * policy_decision,
      gpointer user_data);

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

      bool edit_mode = false;

    private:
      /* message manipulation and location */
      void scroll_to_message (refptr<Message>, bool = false);
      void update_focus_to_view ();
      void update_focus_status ();
      void focus_next ();
      void focus_previous ();

      void focus_next_element ();
      void focus_previous_element ();

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

          enum ElementType {
            Empty,
            Address,
            Part,
            Attachment
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
      bool element_action (char);

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

      atomic<bool> wk_loaded;

      /* rendering */
      void render ();
      void render_messages ();
      void add_message (refptr<Message>);
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

      /* attachments */
      void insert_attachments (refptr<Message>, WebKitDOMHTMLElement *);

      void set_attachment_src (refptr<Chunk>,
          refptr<Glib::ByteArray>,
          WebKitDOMHTMLImageElement *);

      refptr<Gdk::Pixbuf> attachment_icon;

      string assemble_data_uri (ustring, gchar *&, gsize);

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
      /* event wrappers */
      bool on_load_changed (
          GtkWidget *,
          GParamSpec *);

      void on_scroll_vadjustment_changed();
      bool in_scroll = false;
      refptr<Message> scroll_arg;
      bool            _scroll_when_visible;

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

      void grab_focus ();

      /* mode */
      virtual void grab_modal () override;
      virtual void release_modal () override;
      virtual ModeHelpInfo * key_help ();

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;

    public:
      /* the tv is ready */
      typedef sigc::signal <void> type_signal_ready;
      type_signal_ready signal_ready ();

      void emit_ready ();

      /* action on element */
      typedef sigc::signal <void, unsigned int, char> type_element_action;
      type_element_action signal_element_action ();

      void emit_element_action (unsigned int element, char action);

    protected:
      type_signal_ready m_signal_ready;
      type_element_action m_element_action;
  };
}

