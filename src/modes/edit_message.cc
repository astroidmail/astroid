# include <iostream>
# include <vector>
# include <algorithm>
# include <random>
# include <ctime>

# include <gtkmm.h>
# include <gdk/gdkx.h>
# include <gtkmm/socket.h>
# include <giomm/socket.h>
# include <glibmm/iochannel.h>

# include "astroid.hh"
# include "config.hh"
# include "account_manager.hh"
# include "edit_message.hh"

using namespace std;

namespace Astroid {
  int EditMessage::edit_id = 0;

  EditMessage::EditMessage () {
    editor_config = astroid->config->config.get_child ("editor");

    tab_widget = new Gtk::Label ("New message");

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("ui/edit-message.glade", "box_message");

    builder->get_widget ("box_message", box_message);

    builder->get_widget ("from_combo", from_combo);
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

    /* set up message id and random server name for gvim */
    id = edit_id++;

    char hostname[1024];
    hostname[1024] = 0;
    gethostname (hostname, 1023);

    int pid = getpid ();

    msg_time = time(0);
    const string _chars = "abcdefghijklmnopqrstuvwxyz1234567890";
    random_device rd;
    mt19937 g(rd());

    int len = 10;
    for (int i = 0; i < len; i++)
      vim_server += _chars[g() % _chars.size()];

    vim_server = ustring::compose ("%2-astroid-%1-%3-%5@%4", id,
        msg_time, vim_server, hostname, pid);

    if (msg_id == "") {
      msg_id = vim_server;
    }

    cout << "em: msg id: " << msg_id << endl;
    cout << "em: vim server name: " << vim_server << endl;

    /* gvim settings */
    double_esc_deactivates  = editor_config.get <bool>("gvim.double_esc_deactivates");
    default_cmd_on_enter    = editor_config.get <string>("gvim.default_cmd_on_enter");
    gvim_cmd                = editor_config.get <string>("gvim.cmd");
    gvim_args               = editor_config.get <string>("gvim.args");

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

    /* from combobox */
    from_store = Gtk::ListStore::create (from_columns);
    from_combo->set_model (from_store);
    from_combo->set_entry_text_column (from_columns.name_and_address);

    int no = 0;
    for (Account &a : accounts->accounts) {
      auto row = *(from_store->append ());
      row[from_columns.name_and_address] = a.full_address();
      row[from_columns.no] = no++;

      if (a.isdefault) {
        from_combo->set_active (no-1);
      }
    }

    in_edit = true;
    activate_field (To);
  }

  EditMessage::~EditMessage () {
    cout << "em: deconstruct." << endl;
  }

  void EditMessage::socket_realized ()
  {
    cout << "em: socket realized." << endl;

    if (!vim_started) {

      ustring cmd = ustring::compose ("%1 %2 --servername %3 --socketid %4",
          gvim_cmd, gvim_args, vim_server, editor_socket->get_id ());
      cout << "em: starting gvim: " << cmd << endl;
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

      cout << "em: focus editor." << endl;
      if (in_edit) {
        editor_img->set_from_icon_name ("go-next", isize);
        editor_socket->set_sensitive (true);
        editor_socket->set_can_focus (true);

        /* TODO: this should probably only be done the first time,
         *       in subsequent calls it should be sufficient to
         *       just do grab_focus(). it probably works because
         *       gvim only has one widget.
         *
         * Also, this requires GTK to be patched as in:
         * https://bugzilla.gnome.org/show_bug.cgi?id=729248
         * https://mail.gnome.org/archives/gtk-list/2014-May/msg00000.html
         *
         * as far as I can see, there are no other way to
         * programatically set focus to a widget inside an embedded
         * child (except for the user to press Tab).
         */
        gtk_socket_focus_forward (editor_socket->gobj ());

        esc_count     = 0;
        editor_active = true;

        vim_send (default_cmd_on_enter);
      } else {
        editor_img->set_from_icon_name ("media-playback-stop", isize);
      }


    } else if (f == From) {
      if (in_edit) {
        from_combo->set_sensitive (true);
        fields[current_field]->set_icon_from_icon_name ("go-next");
        fields[current_field]->set_sensitive (true);
        fields[current_field]->set_position (-1);
      } else {
        fields[current_field]->set_icon_from_icon_name ("media-playback-stop");
      }
      from_combo->grab_focus ();
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

    /* reset from combo */
    from_combo->popdown ();
    from_combo->set_sensitive (false);

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
            if ((double_esc_deactivates && esc_count == 1) || event->state & GDK_SHIFT_MASK) {
              if (in_edit) {
                in_edit = false;
                activate_field (current_field);
              }

              return true;

            } else {
              esc_count++;
            }

            return false;
          }
        default:
          esc_count = 0;
          return false; // let editor handle
      }
    } else {
      switch (event->keyval) {
        /*
        case GDK_KEY_Right:
          if (current_field == From && !in_edit) {
            from_combo->set_active (from_combo->get_active()+1);
            return true;
          }

        case GDK_KEY_Left:
          if (current_field == From && !in_edit) {
            from_combo->set_active (from_combo->get_active()-1);
            return true;
          }
          */

        case GDK_KEY_Down:
          if (current_field == From && in_edit) {
            from_combo->popup();
            return true;
          }
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

        /* commonly used insert-mode chars */
        case GDK_KEY_a:
        case GDK_KEY_A:
        case GDK_KEY_o:
        case GDK_KEY_O:
        case GDK_KEY_i:
        case GDK_KEY_I:
          if (current_field == Editor) {
            if (!in_edit) {
              in_edit = true;
              activate_field (current_field);
            }
            vim_send (ustring(1, char(event->keyval)));
            return true;
          }

        default:
          return false; // let active field handle event
      }
    }
  }

  void EditMessage::vim_send (ustring keys) {
    ustring cmd = ustring::compose ("%1 --servername %2 --remote-send '%3'", gvim_cmd, vim_server, keys);
    cout << "em: to vim: " << cmd << endl;
    Glib::spawn_command_line_async (cmd.c_str());
  }


  void EditMessage::grab_modal () {
    add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

}

