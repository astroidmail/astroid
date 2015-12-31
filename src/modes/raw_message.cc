# include <iostream>
# include <fstream>

# include "astroid.hh"
# include "raw_message.hh"
# include "message_thread.hh"
# include "log.hh"

using namespace std;

namespace Astroid {
  RawMessage::RawMessage (MainWindow * mw) : Mode (mw) {
    scroll.add (tv);
    pack_start (scroll, true, true, 5);

    tv.set_editable (false);
    tv.set_can_focus (false);
    tv.set_left_margin (10);
    tv.set_right_margin (10);
    tv.set_wrap_mode (Gtk::WRAP_WORD_CHAR);

    Pango::FontDescription fnt;
    fnt.set_family ("monospace");
    tv.override_font (fnt);

    show_all_children ();

    keys.title = "Raw message";
    keys.register_key ("j", { Key (GDK_KEY_Down) },
        "raw.down",
        "Move down",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_step_increment ());
          return true;
        });

    keys.register_key ("J",
        "raw.page_down",
        "Page down",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());
          return true;
        });

    keys.register_key ("k", { Key (GDK_KEY_Up) },
        "raw.up",
        "Move up",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_step_increment ());
          return true;
        });

    keys.register_key ("K",
        "raw.page_up",
        "Page up",
        [&] (Key) {
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) },
        "raw.home",
        "Scroll home",
        [&] (Key) {
          /* select top */
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_lower());
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) },
        "raw.end",
        "Scroll to end",
        [&] (Key) {
          /* select end */
          auto adj = tv.get_vadjustment ();
          adj->set_value (adj->get_upper());
          return true;
        });
  }

  RawMessage::RawMessage (MainWindow * mw, const char * fname) : RawMessage (mw) {
    stringstream l ("Raw message: ");
    l << fname;
    set_label (l.str());

    /* load message source */
    log << info << "rm: loading message from file: " << fname << endl;
    ifstream f (fname);

    /* TODO: convert to valid utf-8 */
    refptr<Gtk::TextBuffer> buf = tv.get_buffer ();
    stringstream s;
    s << f.rdbuf ();
    buf->set_text (s.str());
  }

  RawMessage::RawMessage (MainWindow *mw, refptr<Message> _msg) : RawMessage (mw) {
    msg = _msg;

    set_label ("Raw message: " + msg->subject);

    /* load message source */
    log << info << "rm: loading message.. " << endl;

    refptr<Gtk::TextBuffer> buf = tv.get_buffer ();
    stringstream s;
    s << msg->contents ()->get_data ();
    buf->set_text (s.str());
  }

  void RawMessage::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void RawMessage::release_modal () {
    remove_modal_grab ();
  }
}

