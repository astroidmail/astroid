# include <iostream>
# include <vector>
# include <algorithm>

# include <gtkmm.h>
# include <gdk/gdkx.h>
# include <gtkmm/socket.h>
# include <giomm/socket.h>
# include <glibmm/iochannel.h>

# include "astroid.hh"
# include "account_manager.hh"
# include "edit_message.hh"

using namespace std;

namespace Astroid {

  EditMessage::EditMessage () {
    tab_widget = new Gtk::Label ("New message");

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("ui/edit-message.glade", "box_message");

    builder->get_widget ("box_message", box_message);

    builder->get_widget ("from", from);
    builder->get_widget ("to", to);
    builder->get_widget ("cc", cc);
    builder->get_widget ("bcc", bcc);
    builder->get_widget ("subject", subject);

    builder->get_widget ("editor_box", editor_box);
    builder->get_widget ("editor_img", editor_img);

    /* must be in the same order as enum Field */
    fields.push_back (from);
    fields.push_back (to);
    fields.push_back (cc);
    fields.push_back (bcc);
    fields.push_back (subject);

    pack_start (*box_message, true, 5);

    /* set up io socket for communication with vim */
    service = Gio::ThreadedSocketService::create (1);
    service->add_inet_port (12345);
    service->signal_run ().connect (sigc::mem_fun (*this,
          &EditMessage::on_incoming));
    service->start ();

    /* gtk::socket:
     * http://stackoverflow.com/questions/13359699/pyside-embed-vim
     * https://developer.gnome.org/gtkmm-tutorial/stable/sec-plugs-sockets-example.html.en
     * https://mail.gnome.org/archives/gtk-list/2011-January/msg00041.html
     */
    editor_socket = Gtk::manage(new Gtk::Socket ());
    editor_socket->signal_plug_added ().connect (
        sigc::mem_fun(*this, &EditMessage::plug_added) );
    editor_socket->signal_plug_removed ().connect (
        sigc::mem_fun(*this, &EditMessage::plug_removed) );

    editor_socket->signal_realize ().connect (
        sigc::mem_fun(*this, &EditMessage::socket_realized) );

    editor_box->pack_start (*editor_socket, true, 5);
    show_all ();

    /* defaults */
    accounts = astroid->accounts;

    from->set_text (accounts->accounts[accounts->default_account].full_address());

    activate_field (From);
  }

  EditMessage::~EditMessage () {
    cout << "em: deconstruct." << endl;
    sconn->close ();

    service->stop ();
    service->close ();
  }

  bool EditMessage::on_incoming (
      const refptr<Gio::SocketConnection>& socket,
      const refptr<Glib::Object> &obj) {
    cout << "em: got incoming." << endl;

    sconn = socket;
    is    = socket->get_input_stream ();
    os    = socket->get_output_stream ();

    while (1) {
      char buf[300];
      try {
        int n = is->read (buf, 290);
        buf[n] = 0;
        cout << "read: " << buf << endl;

        if (n <= 0) break;
      } catch (Gio::Error e) {
        cout << "em: vim connection closed: " << e.what () << endl;
        break;
      }
    }

    return false;
  }

  /*
  bool EditMessage::editor_listener (Glib::IOCondition condition) {
    ustring s;
    cout << "em: channel, got: ";
    Glib::IOStatus state;
    channel->read_line (s);
    cout << s;
    cout << endl;
    return true;
  }
  */

  void EditMessage::socket_realized ()
  {
    cout << "em: socket realized." << endl;

    if (!vim_started) {
      cout << "em: starting gvim.." << endl;

      ustring cmd = ustring::compose ("gvim --servername test -nb:127.0.0.1:12345:asdf --socketid %1", editor_socket->get_id ());
      Glib::spawn_command_line_async (cmd.c_str());
      vim_started = true;
    }
  }

  void EditMessage::plug_added () {
    cout << "em: gvim connected" << endl;
  }

  bool EditMessage::plug_removed () {
    cout << "em: gvim disconnected" << endl;
    return true;
  }

  void EditMessage::activate_field (Field f) {
    cout << "em: activate field: " << f << endl;
    reset_fields ();

    current_field = f;

    if (f == Editor) {
      Gtk::IconSize isize  (Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);
      if (in_edit) {
        //editor_box->set_sensitive (true);
        //editor_socket->set_sensitive (true);
        editor_img->set_from_icon_name ("go-next", isize);
      } else {
        editor_img->set_from_icon_name ("media-playback-stop", isize);
      }

      cout << "em: focus editor.." << endl;
      if (in_edit) {
        editor_socket->set_sensitive (true);
        editor_socket->set_can_focus (true);

        gtk_socket_focus_forward (editor_socket->gobj ());

        editor_active = true;
      }


    } else {
      if (in_edit) {
        fields[current_field]->set_icon_from_icon_name ("go-next");
        fields[current_field]->set_sensitive (true);
        fields[current_field]->set_position (-1);
      } else {
        fields[current_field]->set_icon_from_icon_name ("media-playback-stop");
      }
      fields[current_field]->grab_focus ();
    }

    /* update tab in case something changed */
    if (subject->get_text ().size () > 0) {
      ((Gtk::Label*)tab_widget)->set_text ("New message: " + subject->get_text ());
    }
  }

  void EditMessage::reset_fields () {
    for_each (fields.begin (),
              fields.end (),
              [&](Gtk::Entry * e) {
                reset_entry (e);
              });

    /* reset editor */
    Gtk::IconSize isize  (Gtk::BuiltinIconSize::ICON_SIZE_BUTTON);
    editor_img->set_from_icon_name ("", isize);
    int w, h;
    Gtk::IconSize::lookup (isize, w, h);
    editor_img->set_size_request (w, h);

    editor_active = false;
    editor_socket->set_sensitive (false);
    editor_socket->set_can_focus (false);
  }

  void EditMessage::reset_entry (Gtk::Entry *e) {
    e->set_icon_from_icon_name ("");
    e->set_activates_default (true);
    if (current_field != Field::Editor)
      fields[current_field]->set_sensitive (false);
  }

  bool EditMessage::on_key_press_event (GdkEventKey * event) {
    cout << "em: got key press" << endl;
    if (editor_active) {
      switch (event->keyval) {
        case GDK_KEY_Escape:
          {
            if (event->state & GDK_SHIFT_MASK) {
              if (in_edit) {
                in_edit = false;
                activate_field (current_field);
              }
            }
            return true;
          }
        default:
          return false; // let editor handle
      }
    } else {
      switch (event->keyval) {
        case GDK_KEY_Down:
        case GDK_KEY_j:
          if (in_edit) return false; // otherwise act as Tab
        case GDK_KEY_Tab:
          {
            activate_field ((Field) ((current_field+1) % ((int)no_fields)));
            return true;
          }

        case GDK_KEY_Up:
        case GDK_KEY_k:
          if (in_edit) {
            return false;
          } else {
            activate_field ((Field) (((current_field)+((int)no_fields)-1) % ((int)no_fields)));
            return true;
          }

        case GDK_KEY_Return:
          {
            if (!in_edit) {
              in_edit = true;
              activate_field (current_field);
            } else {
              // go to next field
              activate_field ((Field) ((current_field+1) % ((int)no_fields)));
            }
            return true;
          }

        case GDK_KEY_Escape:
          {
            if (in_edit) {
              in_edit = false;
              activate_field (current_field);
            }
            return true;
          }

        default:
          return false; // if from field, field will handle event
      }
    }
  }


  void EditMessage::grab_modal () {
    add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

}

