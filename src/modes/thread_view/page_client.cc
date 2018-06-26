# include "page_client.hh"
# include "utils/resource.hh"

# include <webkit2/webkit2.h>
# include <unistd.h>
# include <gtkmm.h>
# include <glib.h>
# include <glibmm.h>
# include <giomm.h>
# include <giomm/socket.h>
# include <gmime/gmime.h>
# include "utils/gmime/gmime-compat.h"
# include <boost/filesystem.hpp>
# include <iostream>
# include <thread>
# include <algorithm>

# include "astroid.hh"
# include "modes/thread_view/webextension/ae_protocol.hh"
# include "modes/thread_view/webextension/dom_utils.hh"
# include "messages.pb.h"

# include "astroid.hh"
# include "config.hh"
# include "thread_view.hh"
# include "message_thread.hh"
# include "chunk.hh"
# include "utils/utils.hh"
# include "utils/address.hh"
# include "utils/vector_utils.hh"
# include "utils/ustring_utils.hh"
# include "utils/gravatar.hh"

# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

using namespace boost::filesystem;

namespace Astroid {
  int PageClient::id = 0;

  PageClient::PageClient () {

    id++;
    ready = false;

    /* load attachment icon */
    Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
    attachment_icon = theme->load_icon (
        "mail-attachment-symbolic",
        ATTACHMENT_ICON_WIDTH,
        Gtk::ICON_LOOKUP_USE_BUILTIN );

    extension_connect_id = g_signal_connect (webkit_web_context_get_default (),
        "initialize-web-extensions",
        G_CALLBACK (PageClient_init_web_extensions),
        (gpointer) this);

  }

  extern "C" void PageClient_init_web_extensions (
      WebKitWebContext * context,
      gpointer           user_data) {

    ((PageClient *) user_data)->init_web_extensions (context);
  }

  PageClient::~PageClient () {
    LOG (debug) << "pc: destruct";
    g_signal_handler_disconnect (webkit_web_context_get_default (),
        extension_connect_id);

    reader_run = false;

    LOG (debug) << "pc: closing";

    /* if (reader_cancel) */
    /*   reader_cancel->cancel (); */
    /* reader_t.join (); */

    istream.clear ();
    ostream.clear ();

    ext->close ();
    srv->close ();
  }

  void PageClient::init_web_extensions (WebKitWebContext * context) {

    /* add path to Astroid web extension */
# ifdef DEBUG
    LOG (warn) << "pc: adding " << Resource::get_exe_dir ().c_str () << " to web extension search path.";

    webkit_web_context_set_web_extensions_directory (
        context,
        Resource::get_exe_dir().c_str());

# else
    // TODO: Add install path
    LOG (error) << "pc: WebKit2 only works in DEBUG builds.";
# endif

    /* set up unix socket */
    LOG (warn) << "pc: id: " << id;

    // TODO: do-while socket_addr path exists
    socket_addr = ustring::compose ("%1/sockets/astroid.%2.%3",
        astroid->standard_paths ().runtime_dir.c_str(),
        id,
        UstringUtils::random_alphanumeric (30));

    refptr<Gio::UnixSocketAddress> addr = Gio::UnixSocketAddress::create (socket_addr,
        Gio::UNIX_SOCKET_ADDRESS_ABSTRACT);

    refptr<Gio::SocketAddress> eaddr;

    LOG (debug) << "pc: socket: " << addr->get_path ();

    mode_t p = umask (0077);
    srv = Gio::SocketListener::create ();
    srv->add_address (addr, Gio::SocketType::SOCKET_TYPE_STREAM,
      Gio::SocketProtocol::SOCKET_PROTOCOL_DEFAULT,
      eaddr);

    /* listen */
    srv->accept_async (sigc::mem_fun (this, &PageClient::extension_connect));
    umask (p);

    /* send socket address (TODO: include key) */
    GVariant * gaddr = g_variant_new_string (addr->get_path ().c_str ());

    webkit_web_context_set_web_extensions_initialization_user_data (
        context,
        gaddr);
  }

  void PageClient::extension_connect (refptr<Gio::AsyncResult> &res) {
    LOG (warn) << "pc: got extension connect";

    ext = refptr<Gio::UnixConnection>::cast_dynamic (srv->accept_finish (res));

    istream = ext->get_input_stream ();
    ostream = ext->get_output_stream ();

    /* setting up reader */
    /* reader_cancel = Gio::Cancellable::create (); */
    /* reader_t = std::thread (&PageClient::reader, this); */

    ready = true;

    if (thread_view->wk_loaded) {
      Glib::signal_idle().connect_once (
        sigc::mem_fun (thread_view, &ThreadView::on_ready_to_render));
    }
  }

  void PageClient::handle_ack (const AstroidMessages::Ack & ack) {
      LOG (debug) << "pc: got ack (s: " << ack.success () << ") , focus: " << ack.focus().mid () << ", e: " << ack.focus().element ();

      if (!ack.focus().mid ().empty () && ack.focus().element () >= 0) {
        thread_view->focused_message = *std::find_if (
            thread_view->mthread->messages.begin (),
            thread_view->mthread->messages.end (),
            [&] (auto &m) { return ack.focus().mid () == m->safe_mid (); });

        thread_view->state[thread_view->focused_message].current_element = ack.focus().element ();

        thread_view->unread_check ();
      }
  }

  void PageClient::reader () {
    LOG (debug) << "pc: reader started.";
    while (reader_run) {
      LOG (debug) << "pc: reader waiting..";
      gsize read = 0;

      /* read size of message */
      gsize sz;

      try {
        read = istream->read ((char*)&sz, sizeof (sz), reader_cancel); // blocking
      } catch (Gio::Error &ex) {
        LOG (warn) << "pc: " << ex.what ();
        reader_run = false;
        break;
      }

      if (read != sizeof(sz)) {
        LOG (debug) << "pc: reader: could not read size.";
        break;
      }

      if (sz > AeProtocol::MAX_MESSAGE_SZ) {
        LOG (error) << "pc: reader: message exceeds max size.";
        break;
      }

      /* read message type */
      AeProtocol::MessageTypes mt;
      read = istream->read ((char*)&mt, sizeof (mt));
      if (read != sizeof (mt)) {
        LOG (debug) << "pc: reader: size of message type too short: " << sizeof(mt) << " != " << read;
        break;
      }

      /* read message */
      gchar buffer[sz + 1]; buffer[sz] = '\0';
      bool s = istream->read_all (buffer, sz, read);

      if (!s) {
        LOG (debug) << "pc: reader: error while reading message (size: " << sz << ")";
        break;
      }

      /* parse message */
      switch (mt) {
        case AeProtocol::MessageTypes::Debug:
          {
            AstroidMessages::Debug m;
            m.ParseFromString (buffer);
            Glib::signal_idle().connect_once (
                [&,m] () {
                  LOG (debug) << "pc: ae: " << m.msg ();
                });
          }
          break;

        case AeProtocol::MessageTypes::Focus:
          {
            AstroidMessages::Focus f;
            f.ParseFromString (buffer);

            /* update focused element in state */
            Glib::signal_idle().connect_once (
                [&,f] () {
                  LOG (debug) << "pc: got focus event: " << f.mid () << ", e: " << f.element ();
                  thread_view->focused_message = *std::find_if (
                      thread_view->mthread->messages.begin (),
                      thread_view->mthread->messages.end (),
                      [&] (auto &m) { return f.mid () == m->safe_mid (); });

                  thread_view->state[thread_view->focused_message].current_element = f.element ();
                });
          }
          break;

        default:
          break; // unknown message
      }
    }

    LOG (debug) << "pc: reader exit.";
  }

  void PageClient::load () {
    /* load style sheet */
    LOG (debug) << "pc: sending page..";
    AstroidMessages::Page s;
    s.set_css  (thread_view->theme.thread_view_css.c_str ());
    s.set_part_css (thread_view->theme.part_css.c_str ());
    s.set_html (thread_view->theme.thread_view_html.c_str ());

    /* send allowed URIs */
    if (enable_gravatar) {
      s.add_allowed_uris ("https://www.gravatar.com/avatar/");
    }

# ifndef DISABLE_PLUGINS
    /* get plugin allowed uris */
    std::vector<ustring> puris = thread_view->plugins->get_allowed_uris ();
    if (puris.size() > 0) {
      LOG (debug) << "pc: plugin allowed uris: " << VectorUtils::concat_tags (puris);
      for (auto &p : puris) {
        s.add_allowed_uris (p);
      }
    }
# endif

    AeProtocol::send_message_sync (AeProtocol::MessageTypes::Page, s, ostream, m_ostream, istream, m_istream);
  }

  void PageClient::clear_messages () {
    LOG (debug) << "pc: clear messages..";
    AstroidMessages::ClearMessage c;
    c.set_yes (true);
    AeProtocol::send_message_sync (AeProtocol::MessageTypes::ClearMessages, c, ostream, m_ostream, istream, m_istream);
  }

  void PageClient::update_state () {
    /* Synchronize state structure between ThreadView and Extension. Only a minimal structure
     * is sent to the extension.
     *
     * This must be called every time the messages in the thread are changed in a way that effects:
     *
     * * Order
     * * Adding / removing
     * * Changing elements
     *
     */
    LOG (debug) << "pc: sending state..";
    AstroidMessages::State state;

    state.set_edit_mode (thread_view->edit_mode);

    for (refptr<Message> &ms : thread_view->mthread->messages) {
      AstroidMessages::State::MessageState * m = state.add_messages ();

      m->set_mid (ms->safe_mid ());
      m->set_level (ms->level);

      for (auto &e : thread_view->state[ms].elements) {
        AstroidMessages::State::MessageState::Element * _e = m->add_elements ();

        auto ref = _e->GetReflection();
        ref->SetEnumValue (_e, _e->GetDescriptor()->FindFieldByName("type"), e.type);
        _e->set_id (e.id);
        _e->set_sid (e.element_id ());
        _e->set_focusable (e.focusable);
      }
    }

    handle_ack (
      AeProtocol::send_message_sync (AeProtocol::MessageTypes::State, state, ostream, m_ostream, istream, m_istream)
      );
  }

  void PageClient::update_indent_state (bool indent) {
    LOG (debug) << "pc: sending indent..";
    AstroidMessages::Indent msg;
    msg.set_bogus ("asdfadsf");
    msg.set_indent (indent);
    handle_ack (
      AeProtocol::send_message_sync (AeProtocol::MessageTypes::Indent, msg, ostream, m_ostream, istream, m_istream)
    );
  }

  void PageClient::set_marked_state (refptr<Message> m, bool marked) {
    AstroidMessages::Mark msg;
    msg.set_mid (m->safe_mid ());
    msg.set_marked (marked);

    handle_ack (
      AeProtocol::send_message_sync (AeProtocol::MessageTypes::Mark, msg, ostream, m_ostream, istream, m_istream)
      );
  }

  void PageClient::set_hidden_state (refptr<Message> m, bool hidden) {
    LOG (debug) << "pc: set hidden state";
    AstroidMessages::Hidden msg;
    msg.set_mid (m->safe_mid ());
    msg.set_hidden (hidden);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Hidden, msg, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::set_focus (refptr<Message> m, unsigned int e) {
    if (m) {
      LOG (debug) << "pc: focusing: " << m->safe_mid () << ": " << e;
      AstroidMessages::Focus msg;
      msg.set_mid (m->safe_mid ());
      msg.set_focus (true);
      msg.set_element (e);

      handle_ack (
          AeProtocol::send_message_sync (AeProtocol::MessageTypes::Focus, msg, ostream, m_ostream, istream, m_istream)
          );
    } else {
      LOG (warn) << "pc: tried to focus unset message";
    }
  }

  void PageClient::toggle_part (refptr<Message> m, refptr<Chunk> c, ThreadView::MessageState::Element el) {
    /* hides current sibling part and shows the focused one */
    LOG (debug) << "pc: toggling part: " << m->safe_mid () << ", " << c->id;

    /* update state */
    std::find_if (
      thread_view->state[m].elements.begin (),
      thread_view->state[m].elements.end (),
      [&] (auto e) { return e.id == el.id; })->focusable = false;


    /* make siblings focusable */
    for (auto &c : c->siblings) {
      std::find_if (
        thread_view->state[m].elements.begin (),
        thread_view->state[m].elements.end (),
        [&] (auto e) { return e.id == c->id; })->focusable = true;
    }

    /* update state */
    update_state ();

    /* send updated message */
    update_message (m);
  }

  void PageClient::remove_message (refptr<Message> m) {
    AstroidMessages::Message msg;
    msg.set_mid (m->safe_mid()); // just mid.
    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::RemoveMessage, msg, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::add_message (refptr<Message> m) {
    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::AddMessage, make_message (m), ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::update_message (refptr<Message> m) {
    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::UpdateMessage, make_message (m, true), ostream, m_ostream, istream, m_istream)
        );
  }

  AstroidMessages::Message PageClient::make_message (refptr<Message> m, bool keep_state) {
    typedef ThreadView::MessageState MessageState;
    AstroidMessages::Message msg;

    msg.set_mid (m->safe_mid());

    Address sender (m->sender);
    msg.mutable_sender()->set_name (sender.fail_safe_name ());
    msg.mutable_sender()->set_email (sender.email ());
    msg.mutable_sender ()->set_full_address (sender.full_address ());

    for (Address &recipient: AddressList(m->to()).addresses) {
      AstroidMessages::Address * a = msg.mutable_to()->add_addresses();
      a->set_name (recipient.fail_safe_name ());
      a->set_email (recipient.email ());
      a->set_full_address (recipient.full_address ());
    }

    for (Address &recipient: AddressList(m->cc()).addresses) {
      AstroidMessages::Address * a = msg.mutable_cc()->add_addresses();
      a->set_name (recipient.fail_safe_name ());
      a->set_email (recipient.email ());
      a->set_full_address (recipient.full_address ());
    }

    for (Address &recipient: AddressList(m->bcc()).addresses) {
      AstroidMessages::Address * a = msg.mutable_bcc()->add_addresses();
      a->set_name (recipient.fail_safe_name ());
      a->set_email (recipient.email ());
      a->set_full_address (recipient.full_address ());
    }

    msg.set_date_pretty (m->pretty_date ());
    msg.set_date_verbose (m->pretty_verbose_date (true));

    msg.set_subject (m->subject);

    msg.set_patch (m->is_patch ());

    /* tags */
    {
      unsigned char cv[] = { 0xff, 0xff, 0xff };

      ustring tags_s;

# ifndef DISABLE_PLUGINS
      if (!thread_view->plugins->format_tags (m->tags, "#ffffff", false, tags_s)) {
#  endif

        tags_s = VectorUtils::concat_tags_color (m->tags, false, 0, cv);

# ifndef DISABLE_PLUGINS
      }
# endif

      msg.set_tag_string (tags_s);

      for (ustring &tag : m->tags) {
        msg.add_tags (tag);
      }
    }

    /* avatar */
    {
      ustring uri = "";
      auto se = Address(m->sender);
# ifdef DISABLE_PLUGINS
      if (false) {
# else
      if (thread_view->plugins->get_avatar_uri (se.email (), Gravatar::DefaultStr[Gravatar::Default::RETRO], 48, m, uri)) {
# endif
        ; // all fine, use plugins avatar
      } else {
        if (enable_gravatar) {
          uri = Gravatar::get_image_uri (se.email (),Gravatar::Default::RETRO , 48);
        }
      }

      msg.set_gravatar (uri);
    }

    /* set preview */
    {
      ustring bp = m->viewable_text (false, false);
      if (static_cast<int>(bp.size()) > MAX_PREVIEW_LEN)
        bp = bp.substr(0, MAX_PREVIEW_LEN - 3) + "...";

      while (true) {
        size_t i = bp.find ("<br>");

        if (i == ustring::npos) break;

        bp.erase (i, 4);
      }

      msg.set_preview (Glib::Markup::escape_text (bp));
    }

    if (astroid->config().get<std::string> ("thread_view.preferred_type") == "plain" &&
        astroid->config().get<bool> ("thread_view.preferred_html_only")) {
      /* check if we have a preferred part - and open first viewable if not */
      bool found_preferred = false;
      for (auto &c : m->all_parts ()) {
        if (c->preferred) {
          found_preferred = true;
          break;
        }
      }

      /* take first viewable */
      if (!found_preferred) {
        for (auto &c : m->all_parts ()) {
          if (c->viewable) {
            c->preferred = true;
            break;
          }
        }
      }
    }

    /* build structure */
    msg.set_allocated_root (build_mime_tree (m, m->root, true, false, keep_state));

    /* add MIME messages */
    for (refptr<Chunk> &c : m->mime_messages ()) {
      auto _c = msg.add_mime_messages ();
      auto _n = build_mime_tree (m, c, false, true, keep_state);
      *_c = *_n;
      delete _n;

      if (!keep_state) {
        // add MIME message to message state
        MessageState::Element e (MessageState::ElementType::MimeMessage, c->id);
        thread_view->state[m].elements.push_back (e);
      }
    }

    /* add attachments */
    for (refptr<Chunk> &c : m->attachments ()) {

      auto _c = msg.add_attachments ();
      auto _n = build_mime_tree (m, c, false, true, keep_state);

      *_c = *_n;
      delete _n;

      _c->set_thumbnail (get_attachment_thumbnail (c));

      if (!keep_state) {
        // add attachment to message state
        MessageState::Element e (MessageState::ElementType::Attachment, c->id);
        thread_view->state[m].elements.push_back (e);
      }
    }

    return msg;
  }

  AstroidMessages::Message::Chunk * PageClient::build_mime_tree (refptr<Message> m, refptr<Chunk> c, bool root, bool shallow, bool keep_state)
  {
    typedef ThreadView::MessageState MessageState;

    ustring mime_type;
    if (c->content_type) {
      mime_type = ustring(g_mime_content_type_get_mime_type (c->content_type));
    } else {
      mime_type = "application/octet-stream";
    }

    LOG (debug) << "create message part: " << c->id << " (siblings: " << c->siblings.size() << ") (kids: " << c->kids.size() << ")" <<
      " (attachment: " << c->attachment << ")" << " (viewable: " << c->viewable << ")" << " (mimetype: " << mime_type << ")";

    auto part = new AstroidMessages::Message::Chunk ();

    /* defaults */
    part->set_mime_type (mime_type);
    part->set_content ("");

    part->set_id (c->id);
    part->set_sid (ustring::compose ("%1", c->id));
    part->set_sibling (!c->siblings.empty ());
    part->set_viewable (c->viewable);
    part->set_preferred (c->preferred);
    part->set_attachment (c->attachment);

    part->set_filename (c->get_filename ());
    part->set_size (c->get_file_size ());
    part->set_human_size (Utils::format_size (part->size()));

    // TODO: Add signature state
    // TODO: Add encryption state
    part->set_is_signed (c->issigned);
    part->set_is_encrypted (c->isencrypted);

    if (shallow) return part;

    if (root && c->attachment) {
      /* return empty root part */
      return part;
    } else if (c->attachment) {
      delete part;
      return NULL;
    }

    if (c->viewable) {
      // TODO: Filter code tags
      part->set_content (c->viewable_text (true, true));
    }


    /* Check if we are preferred part or sibling.
     *
     * If not used only a sibling part will be created.
     *
     */
    bool use = false;

    if (c->viewable) {
      if (c->preferred) {
        use = true;
      } else {
        use = false; // create sibling part
      }
    } else {
      /* a part which may contain sub-parts */
      use = true;
    }

    part->set_use (use);


    if (use) {
      if (c->viewable) {
        /* make state element */
        if (!keep_state) {
          MessageState::Element e (MessageState::ElementType::Part, c->id);

          if (c->preferred) {
            // shown
            e.focusable = false;
          } else {
            // hidden by default
            e.focusable = true;
          }

          part->set_focusable (e.focusable);
          thread_view->state[m].elements.push_back (e);
        } else {
          LOG (debug) << "cid: " << c->id;
          part->set_focusable ( thread_view->state[m].get_element_by_id (c->id)->focusable );
        }
      }

      /* recurse into children after first part so that we get the correct order
       * on elements */
      for (auto &k : c->kids) {
        auto ch = build_mime_tree (m, k, false, false, keep_state);
        if (ch != NULL) {
          auto nch = part->add_kids ();
          *nch = *ch;
          delete ch;
        }
      }

    } else {
      /* make state element for sibling part (hidden by default) */
      if (!keep_state) {
        MessageState::Element e (MessageState::ElementType::Part, c->id);
        part->set_focusable (e.focusable);
        thread_view->state[m].elements.push_back (e);
      } else {
        part->set_focusable ( thread_view->state[m].get_element_by_id (c->id)->focusable );
      }
    }

    return part;
  }

  void PageClient::set_warning (refptr<Message> m, ustring txt) {
    AstroidMessages::Info i;
    i.set_mid (m->safe_mid ());
    i.set_warning (true);
    i.set_set (true);
    i.set_txt (txt);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Info, i, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::hide_warning (refptr<Message> m) {
    AstroidMessages::Info i;
    i.set_mid (m->safe_mid ());
    i.set_warning (true);
    i.set_set (false);
    i.set_txt ("");

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Info, i, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::set_info (refptr<Message> m, ustring txt) {
    AstroidMessages::Info i;
    i.set_mid (m->safe_mid ());
    i.set_warning (false);
    i.set_set (true);
    i.set_txt (txt);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Info, i, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::hide_info (refptr<Message> m) {
    AstroidMessages::Info i;
    i.set_mid (m->safe_mid ());
    i.set_warning (false);
    i.set_set (false);
    i.set_txt ("");

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Info, i, ostream, m_ostream, istream, m_istream)
        );
  }

  ustring PageClient::get_attachment_thumbnail (refptr<Chunk> c) { // {{{
    /* set the preview image or icon on the attachment display element */
    const char * _mtype = g_mime_content_type_get_media_type (c->content_type);
    ustring mime_type;
    if (_mtype == NULL) {
      mime_type = "application/octet-stream";
    } else {
      mime_type = ustring(g_mime_content_type_get_mime_type (c->content_type));
    }

    LOG (debug) << "tv: set attachment, mime_type: " << mime_type << ", mtype: " << _mtype;

    gchar * content;
    gsize   content_size;
    ustring image_content_type;

    if ((_mtype != NULL) && (ustring(_mtype) == "image")) {
      auto mis = Gio::MemoryInputStream::create ();

      refptr<Glib::ByteArray> data = c->contents ();
      mis->add_data (data->get_data (), data->size ());

      try {

        auto pb = Gdk::Pixbuf::create_from_stream_at_scale (mis, THUMBNAIL_WIDTH, -1, true, refptr<Gio::Cancellable>());
        pb = pb->apply_embedded_orientation ();

        pb->save_to_buffer (content, content_size, "png");
        image_content_type = "image/png";
      } catch (Gdk::PixbufError &ex) {

        LOG (error) << "tv: could not create icon from attachmed image.";
        attachment_icon->save_to_buffer (content, content_size, "png"); // default type is png
        image_content_type = "image/png";
      }
    } else {
      // TODO: guess icon from mime type. Using standard icon for now.

      attachment_icon->save_to_buffer (content, content_size, "png"); // default type is png
      image_content_type = "image/png";
    }

    return DomUtils::assemble_data_uri (image_content_type.c_str (), content, content_size);
  } // }}}

  void PageClient::scroll_down_big () {
    AstroidMessages::Navigate n;
    n.set_direction (AstroidMessages::Navigate_Direction_Down);
    n.set_type (AstroidMessages::Navigate_Type_VisualBig);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::scroll_up_big () {
    AstroidMessages::Navigate n;
    n.set_direction (AstroidMessages::Navigate_Direction_Up);
    n.set_type (AstroidMessages::Navigate_Type_VisualBig);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::focus_next_element (bool force_change) {
    AstroidMessages::Navigate n;

    n.set_direction (AstroidMessages::Navigate_Direction_Down);

    if (force_change) {
      n.set_type (AstroidMessages::Navigate_Type_Element);
    } else {
      n.set_type (AstroidMessages::Navigate_Type_VisualElement);
    }

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::focus_previous_element (bool force_change) {
    AstroidMessages::Navigate n;

    n.set_direction (AstroidMessages::Navigate_Direction_Up);

    if (force_change) {
      n.set_type (AstroidMessages::Navigate_Type_Element);
    } else {
      n.set_type (AstroidMessages::Navigate_Type_VisualElement);
    }

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::focus_next_message () {
    AstroidMessages::Navigate n;

    n.set_direction (AstroidMessages::Navigate_Direction_Down);
    n.set_type (AstroidMessages::Navigate_Type_Message);
    n.set_focus_top (false); // not relevant

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::focus_previous_message (bool focus_top) {
    AstroidMessages::Navigate n;

    n.set_direction (AstroidMessages::Navigate_Direction_Up);
    n.set_type (AstroidMessages::Navigate_Type_Message);
    n.set_focus_top (focus_top);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }

  void PageClient::focus_element (refptr<Message> m, unsigned int e) {
    AstroidMessages::Navigate n;

    n.set_direction (AstroidMessages::Navigate_Direction_Specific);
    n.set_type (AstroidMessages::Navigate_Type_Element);
    n.set_mid (m->safe_mid ());
    n.set_element (e);

    handle_ack (
        AeProtocol::send_message_sync (AeProtocol::MessageTypes::Navigate, n, ostream, m_ostream, istream, m_istream)
        );
  }
}

