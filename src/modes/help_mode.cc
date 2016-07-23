# include <iostream>

# include "main_window.hh"
# include "mode.hh"
# include "help_mode.hh"
# include "astroid.hh"

# include <vector>
# include <tuple>

using namespace std;

namespace Astroid {
  HelpMode::HelpMode (MainWindow * mw) : Mode (mw) {
    set_label ("Help");

    scroll.add (help_text);
    pack_start (scroll, true, true, 5);

    scroll.show_all ();

    /* register keys */
    keys.register_key ("j",
        { Key("C-j"),
          Key (true, false, (guint) GDK_KEY_Down),
          Key(GDK_KEY_Down) },
        "help.down",
        "Scroll down",
        [&] (Key k) {
          if (k.ctrl) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_page_increment ());
          } else {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_step_increment ());
          }

          return true;
        });

    keys.register_key ("k",
        { Key ("C-k"),
          Key (true, false, (guint) GDK_KEY_Up),
          Key (GDK_KEY_Up) },
        "help.up",
        "Scroll up",
        [&] (Key k) {
          if (k.ctrl) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_page_increment ());
          } else {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_step_increment ());
          }

          return true;
        });

    keys.register_key ("K", { Key (GDK_KEY_Page_Up) }, "help.page_up",
        "Page up",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());
          return true;
        });

    keys.register_key ("J", { Key (GDK_KEY_Page_Down) }, "help.page_down",
        "Page down",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());
          return true;
        });

    keys.register_key ("1", { Key (GDK_KEY_Home) }, "help.page_top",
        "Scroll to top",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_lower ());
          return true;
        });

    keys.register_key ("0", { Key (GDK_KEY_End) }, "help.page_end",
        "Scroll to end",
        [&] (Key) {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_upper ());
          return true;
        });

  }

  HelpMode::HelpMode (MainWindow *mw, Mode * m) : HelpMode (mw) {
    show_help (m);
  }

  void HelpMode::show_help (Mode * m) {
    set_label ("Help: " + m->get_keys ()->title);

    ustring header = ustring::compose(
    "<b>Astroid</b> (%1) \n"
    "\n"
    "Gaute Hope &lt;<a href=\"mailto:eg@gaute.vetsj.com\">eg@gaute.vetsj.com</a>&gt; (c) 2014"
    " (<i>Licenced under the GNU GPL v3</i>)\n"
    "<a href=\"https://github.com/astroidmail/astroid\">https://github.com/astroidmail/astroid</a> | <a href=\"mailto:astroidmail@googlegroups.com\">astroidmail@googlegroups.com</a>\n"
    "\n", Astroid::version);

    ustring help = header + generate_help (m);

    help_text.set_markup (help);
  }

  ustring HelpMode::generate_help (Gtk::Widget * w) {
    ustring h;

    MainWindow * mw = dynamic_cast<MainWindow *> (w);
    bool is_mw = (mw != NULL); // we stop here

    Mode * m   = dynamic_cast<Mode *> (w);
    bool is_mode = (m != NULL);


    if (!is_mw) {
      h = generate_help (w->get_parent ());
    }

    if (is_mode) {
      h += "---\n";
      h += "<i>" + m->get_keys ()->title + "</i>\n";

      h += m->get_keys ()->help ();
    }

    if (is_mw) {
      h += "<i>Main window</i>\n";
      h += mw->keys.help ();
    }

    return h;
  }

  void HelpMode::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void HelpMode::release_modal () {
    remove_modal_grab ();
  }
};

