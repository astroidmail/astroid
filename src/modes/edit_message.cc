# include <iostream>
# include <vector>
# include <algorithm>

# include "astroid.hh"
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

    /* must be in the same order as enum Field */
    fields.push_back (from);
    fields.push_back (to);
    fields.push_back (cc);
    fields.push_back (bcc);

    pack_start (*box_message, true, 5);

    show_all ();

    activate_field (From);
  }

  void EditMessage::activate_field (Field f) {
    cout << "em: activate field: " << f << endl;
    reset_fields ();

    current_field = f;

    if (in_edit) {
      fields[current_field]->set_icon_from_icon_name ("go-next");
      fields[current_field]->set_sensitive (true);
    } else {
      fields[current_field]->set_icon_from_icon_name ("media-playback-stop");
    }
    fields[current_field]->grab_focus ();
  }

  void EditMessage::reset_fields () {
    for_each (fields.begin (),
              fields.end (),
              [&](Gtk::Entry * e) {
                reset_entry (e);
              });
  }

  void EditMessage::reset_entry (Gtk::Entry *e) {
    e->set_icon_from_icon_name ("");
    e->set_activates_default (true);
    fields[current_field]->set_sensitive (false);
  }

  bool EditMessage::on_key_press_event (GdkEventKey * event) {
    cout << "em: got key press" << endl;
    switch (event->keyval) {
      case GDK_KEY_j:
        if (in_edit) return false; // otherwise act as Tab
      case GDK_KEY_Tab:
        {
          activate_field ((Field) ((current_field+1) % ((int)fields.size())));
          return true;
        }

      case GDK_KEY_k:
        if (in_edit) {
          return false;
        } else {
          activate_field ((Field) (((current_field)+((int)fields.size())-1) % ((int)fields.size())));
          return true;
        }

      case GDK_KEY_Return:
        {
          if (!in_edit) {
            in_edit = true;
            activate_field (current_field);
          } else {
            // go to next field
            activate_field ((Field) ((current_field+1) % ((int)fields.size())));
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


  void EditMessage::grab_modal () {
    add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

}

