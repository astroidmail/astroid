# include <iostream>

# include <gtkmm.h>

# include "thread_view.hh"
# include "message.hh"


using namespace std;

namespace Gulp {

  void ThreadView::load_thread (ustring _thread_id) {
    thread_id = _thread_id;

  }

  void ThreadView::render () {

  }

  void ThreadView::grab_modal () {

  }

  void ThreadView::release_modal () {

  }

  Gtk::Widget * ThreadView::get_widget () {
    return (Gtk::Widget *) this;
  }
}

