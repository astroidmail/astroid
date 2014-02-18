# include <iostream>

# include <gtkmm.h>

# include "thread_view.hh"
# include "message.hh"


using namespace std;

namespace Astroid {

  ThreadView::ThreadView () {
    tab_widget = new Gtk::Label (thread_id);

    pack_start (scroll, true, true, 5);
    scroll.show_all ();
  }

  void ThreadView::load_thread (ustring _thread_id) {
    thread_id = _thread_id;

    ((Gtk::Label*) tab_widget)->set_text (thread_id);
  }

  void ThreadView::render () {

  }

  void ThreadView::grab_modal () {
    scroll.add_modal_grab ();
    scroll.grab_focus ();
  }

  void ThreadView::release_modal () {
    scroll.remove_modal_grab ();
  }

}

