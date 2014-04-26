# include <iostream>

# include "astroid.hh"
# include "edit_message.hh"

using namespace std;

namespace Astroid {

  EditMessage::EditMessage () {
    tab_widget = new Gtk::Label ("New message");

    Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create_from_file("ui/edit-message.glade", "box_message");

    builder->get_widget ("box_message", box_message);
    pack_start (*box_message, true, 5);

    show_all ();

  }

  void EditMessage::grab_modal () {
    add_modal_grab ();
  }

  void EditMessage::release_modal () {
    remove_modal_grab ();
  }

}

