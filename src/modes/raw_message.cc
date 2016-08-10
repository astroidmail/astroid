# include <iostream>
# include <fstream>
# include <boost/filesystem.hpp>

# include "astroid.hh"
# include "raw_message.hh"
# include "message_thread.hh"
# include "log.hh"

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
    log << info << "rm: loading message from file: " << fname << endl;
    ifstream f (fname);

    refptr<Gtk::TextBuffer> buf = tv.get_buffer ();
    stringstream s;

    /* add filenames */
    s << "Filename: " << fname << endl << endl;

    s << f.rdbuf ();
    std::string _in = s.str ();

    int len    = _in.size ();
    gchar * in = (gchar *) _in.c_str ();

    /* convert */
    gsize read, written;
    GError * err = NULL;
    gchar * out = g_convert_with_fallback (in, len, "UTF-8", "ASCII", NULL,
                                           &read, &written, &err);

    if (out != NULL) {
      buf->set_text ( out, out+written);
    } else {
      log << error << "raw: could not convert: " << in << endl;
    }

    g_free (out);
  }

  RawMessage::RawMessage (MainWindow *mw, refptr<Message> _msg) : RawMessage (mw) {
    msg = _msg;

    set_label ("Raw message: " + msg->subject);

    /* load message source */
    log << info << "rm: loading message.. " << endl;

    refptr<Gtk::TextBuffer> buf = tv.get_buffer ();

    auto c = msg->raw_contents ();

    stringstream s;

    /* add filenames */
    s << "Filename: " << msg->fname << endl << endl;

    s << c->get_data ();
    std::string _in = s.str ();
    int len    = _in.size ();
    gchar * in = (gchar *) _in.c_str ();

    /* convert */
    gsize read, written;
    GError * err = NULL;
    gchar * out = g_convert_with_fallback (in, len, "UTF-8", "ASCII", NULL,
                                           &read, &written, &err);

    if (out != NULL) {
      buf->set_text ( out, out+written);
    } else {
      log << error << "raw: could not convert: " << in << endl;
    }

    g_free (out);
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

