# include <iostream>

# include "main_window.hh"
# include "mode.hh"
# include "fail_mode.hh"
# include "log_view.hh"
# include "astroid.hh"

using namespace std;

namespace Astroid {
  FailMode::FailMode (MainWindow * mw, ustring err) : Mode (mw) {
    set_label ("Oh dear!");

    invincible = true;

    Gtk::VBox * vb = Gtk::manage (new Gtk::VBox ());

    Gtk::Image * im = Gtk::manage (new Gtk::Image ());
    im->set_from_icon_name ("dialog-warning-symbolic", Gtk::ICON_SIZE_DIALOG);
    vb->pack_start (*im, false, false, 5);

    scroll.add (fail_text);
    vb->pack_start (scroll, true, true, 5);

    pack_start (*vb, true, true, 5);

    lv = Gtk::manage (new LogView (mw));
    pack_end (*lv, true, true, 5);

    ustring errmsg = ustring::compose (
        "<i>Oh dear! There seems to have been a incident...</i>\n"
        "\n"
        "<span color=\"pink\">%1</span>\n"
        "\n"
        "See the log below for more information.\n"
        "\n"
        "Please check the wiki (<a href=\"http://https://github.com/gauteh/astroid/wiki\">https://github.com/gauteh/astroid/wiki</a>) if you\n"
        "can find a solution there. If you think this is a bug, please report\n"
        "it to: <a href=\"https://github.com/gauteh/astroid\">https://github.com/gauteh/astroid</a>."
        , err);

    fail_text.set_markup (errmsg);

    show_all ();
  }

  void FailMode::grab_modal () {
    lv->grab_modal ();
  }

  void FailMode::release_modal () {
    lv->release_modal ();
  }
};

