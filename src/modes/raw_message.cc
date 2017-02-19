# include <iostream>
# include <fstream>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "raw_message.hh"
# include "message_thread.hh"
# include "utils/ustring_utils.hh"

using namespace std;
namespace bfs = boost::filesystem;

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

    keys.title = "Raw message"; // {{{
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
    // }}}
  }

  RawMessage::RawMessage (MainWindow * mw, const char * fname, bool _delete)
      : RawMessage (mw)

  {
    delete_on_close = _delete;
    this->fname = fname;

    stringstream l ("Raw message: ");
    l << fname;
    set_label (l.str());

    /* load message source */
    LOG (info) << "rm: loading message from file: " << fname;

    refptr<Gtk::TextBuffer> buf = tv.get_buffer ();
    stringstream s;

    /* add filenames */
    s << "Filename: " << fname << endl << endl;

    ifstream f (fname);
    std::filebuf  * pbuf = f.rdbuf ();
    size_t fsz = pbuf->pubseekoff (0, f.end, f.in);
    pbuf->pubseekpos (0, f.in);

    char * fbuf = new char[fsz];
    pbuf->sgetn (fbuf, fsz);
    f.close ();

    auto cnv = UstringUtils::data_to_ustring (fsz, fbuf);
    delete [] fbuf;

    if (cnv.first) {
      s << cnv.second;
    } else {
      s << "Error: Could not convert input to UTF-8.";
    }

    buf->set_text ( s.str () );
  }

  RawMessage::RawMessage (MainWindow *mw, refptr<Message> _msg) : RawMessage (mw) {
    msg = _msg;

    set_label ("Raw message: " + msg->subject);

    /* load message source */
    LOG (info) << "rm: loading message.. ";

    refptr<Gtk::TextBuffer> buf = tv.get_buffer ();

    auto c = msg->raw_contents ();

    stringstream s;

    /* add filenames */
    s << "Filename: " << msg->fname << endl << endl;

    auto cnv = UstringUtils::bytearray_to_ustring (c);
    if (cnv.first) {
      s << cnv.second;
    } else {
      s << "Error: Could not convert input to UTF-8.";
    }

    buf->set_text ( s.str () );
  }

  RawMessage::~RawMessage () {
    if (delete_on_close) {
      if (bfs::exists (fname)) {
        unlink (fname.c_str ());
      }
    }
  }


  void RawMessage::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void RawMessage::release_modal () {
    remove_modal_grab ();
  }
}

