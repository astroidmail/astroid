# include <iostream>

# include "main_window.hh"
# include "mode.hh"
# include "help_mode.hh"
# include "build_config.hh"

# include <vector>
# include <tuple>

using namespace std;

namespace Astroid {
  HelpMode::HelpMode (MainWindow * mw) : Mode (mw) {
    set_label ("Help");

    scroll.add (help_text);
    pack_start (scroll, true, true, 5);

    scroll.show_all ();
  }

  HelpMode::HelpMode (MainWindow *mw, Mode * m) : HelpMode (mw) {
    show_help (m);
  }

  void HelpMode::show_help (Mode * m) {
    ModeHelpInfo * mhelp = m->key_help ();

    set_label ("Help: " + mhelp->title);

    ustring header =
    "<b>Astroid</b> (" GIT_DESC ") \n"
    "\n"
    "Gaute Hope &lt;<a href=\"mailto:eg@gaute.vetsj.com\">eg@gaute.vetsj.com</a>&gt; (c) 2014"
    " (<i>Licenced under the GNU GPL v3</i>)\n"
    "<a href=\"https://github.com/gauteh/astroid\">https://github.com/gauteh/astroid</a> | <a href=\"mailto:astroidmail@googlegroups.com\">astroidmail@googlegroups.com</a>\n"
    "\n";

    ustring help = header + generate_help (m);

    delete mhelp;

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
      h += "<i>" + m->mode_help_title + "</i>";

      h += m->keys.help ();
    }

    return h;
  }

  bool HelpMode::on_key_press_event (GdkEventKey *event) {
    switch (event->keyval) {
      case GDK_KEY_j:
      case GDK_KEY_Down:
        {
          if (event->state & GDK_CONTROL_MASK) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_page_increment ());
          } else {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() + adj->get_step_increment ());
          }
        }
        return true;

      case GDK_KEY_k:
      case GDK_KEY_Up:
        {
          if (event->state & GDK_CONTROL_MASK) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_page_increment ());
          } else {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_value() - adj->get_step_increment ());
          }
        }
        return true;

      case GDK_KEY_K:
      case GDK_KEY_Page_Up:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() - adj->get_page_increment ());
        }
        return true;

      case GDK_KEY_J:
      case GDK_KEY_Page_Down:
        {
          auto adj = scroll.get_vadjustment ();
          adj->set_value (adj->get_value() + adj->get_page_increment ());
        }
        return true;

      case GDK_KEY_1:
      case GDK_KEY_Home:
        {
          if (!(event->state & GDK_MOD1_MASK)) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_lower ());
            return true;
          }
        }
        break;

      case GDK_KEY_0:
      case GDK_KEY_End:
        {
          if (!(event->state & GDK_MOD1_MASK)) {
            auto adj = scroll.get_vadjustment ();
            adj->set_value (adj->get_upper ());
            return true;
          }
        }
        break;
    }

    return false;
  }

  ModeHelpInfo * HelpMode::key_help () {
    ModeHelpInfo * m = new ModeHelpInfo ();

    m->parent   = Mode::key_help ();
    m->toplevel = false;
    m->title    = "Help";

    m->keys = {
      { "k,j,Up,Down", "Scroll up and down" },
      { "C+direction,J,K,Page+direction", "Page up and down" },
    };

    return m;
  }

  void HelpMode::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void HelpMode::release_modal () {
    remove_modal_grab ();
  }
};

