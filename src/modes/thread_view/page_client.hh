# pragma once

# include <webkit2/webkit2.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <gtkmm.h>
# include <thread>
# include <atomic>

# include "astroid.hh"
# include "thread_view.hh"

# include "messages.pb.h"

namespace Astroid {
  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext *,
      gpointer);

  class PageClient : public sigc::trackable {
    public:
      PageClient (ThreadView *);
      ~PageClient ();

      ThreadView * thread_view;

      void init_web_extensions (WebKitWebContext * context);

      /* ThreadView interface */
      void load ();
      void add_message (refptr<Message> m);
      void update_message (refptr<Message> m, AstroidMessages::UpdateMessage_Type t);
      void remove_message (refptr<Message> m);
      void update_state ();
      void clear_messages ();
      void update_indent_state (bool);
      void allow_remote_resources ();

      void toggle_part (refptr<Message> m, refptr<Chunk>, ThreadView::MessageState::Element);

      void set_marked_state (refptr<Message> m, bool marked);
      void set_hidden_state (refptr<Message> m, bool hidden);

      void set_warning (refptr<Message>, ustring);
      void hide_warning (refptr<Message>);
      void set_info (refptr<Message>, ustring);
      void hide_info (refptr<Message>);

      /* focus and scrolling */
      void set_focus (refptr<Message> m, unsigned int);

      void scroll_down_big ();
      void scroll_up_big ();
      void scroll_down_page ();
      void scroll_up_page ();

      void focus_element (refptr<Message> m, unsigned int);
      void focus_next_element (bool force_change);
      void focus_previous_element (bool force_change);
      void focus_next_message ();
      void focus_previous_message (bool focus_top);
      void update_focus_to_view ();

      bool enable_gravatar = false;

      std::atomic<bool> ready;

    private:
      AstroidMessages::Message  make_message (refptr<Message> m, bool keep_state = false);
      AstroidMessages::Message::Chunk * build_mime_tree (refptr<Message> m, refptr<Chunk> c, bool root, bool shallow, bool keep_state = false);

      ustring get_attachment_thumbnail (refptr<Chunk>);
      ustring get_attachment_data (refptr<Chunk>);

      static const int MAX_PREVIEW_LEN = 80;
      static const int THUMBNAIL_WIDTH        = 150; // px
      static const int ATTACHMENT_ICON_WIDTH  = 35;
      refptr<Gdk::Pixbuf> attachment_icon;

    private:
      static int id;

      ustring socket_addr;

      refptr<Gio::SocketListener> srv;
      refptr<Gio::UnixConnection> ext;
      void extension_connect (refptr<Gio::AsyncResult> &res);
      gulong extension_connect_id;

      refptr<Gio::InputStream>  istream;
      refptr<Gio::OutputStream> ostream;
      std::mutex      m_ostream;
      std::mutex      m_istream;

      std::thread reader_t;
      void        reader ();
      bool        run = true;
      refptr<Gio::Cancellable> reader_cancel;

      void        handle_ack (const AstroidMessages::Ack & ack);
  };

}

