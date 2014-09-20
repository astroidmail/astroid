# include <iostream>

# include "help_mode.hh"

using namespace std;

namespace Astroid {
  HelpMode::HelpMode () {
    set_label ("Help");

    scroll.add (help_text);
    pack_start (scroll, true, true, 5);

    help_text.set_markup (
    "<b>Astroid</b>\n"
    "\n"
    "Gaute Hope &lt;<a href=\"eg@gaute.vetsj.com\">eg@gaute.vetsj.com</a>&gt; (c) 2014"
    " (<i>Licenced under the GNU GPL v3</i>)\n"
    "<a href=\"https://github.com/gauteh/astroid\">https://github.com/gauteh/astroid</a>\n"
    "\n"
    "<i>General keybindings:</i>\n"
    "---\n"
    "<b>F</b> Open search bar, use <i>notmuch</i> query expressions, press ENTER to search.\n"
    "<b>b</b> Go to next tab.\n"
    "<b>B</b> Go to previous tab.\n"
    "<b>Tab</b> Swap between panes.\n"
    "<b>x</b> Close current tab.\n"
    "<b>?</b> Show this help page\n"
    "<b>q</b> Quit.\n"
    "<b>j, Down</b> Down.\n"
    "<b>k, Up</b> Up\n"
    "<b>Page Up, Down</b> Page Up and Down\n"
    "\n"
    "<i>Thread index:</i>\n"
    "---\n"
    "<b>Enter</b> Open thread in the right pane.\n"
    "<b>S+Enter</b> Open thread in new tab.\n"
    "<b>M</b> Load 20 more threads of current search.\n"
    "<b>!</b> Load all threads of current search.\n"
    );

    scroll.show_all ();
  }

  void HelpMode::grab_modal () {
    add_modal_grab ();
    grab_focus ();
  }

  void HelpMode::release_modal () {
    remove_modal_grab ();
  }
};

