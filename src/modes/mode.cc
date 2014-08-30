# include "mode.hh"
# include "log.hh"

namespace Astroid {
  Mode::Mode (bool _interactive) :
    Gtk::Box (Gtk::ORIENTATION_VERTICAL),
    interactive (_interactive)
  {


    /* set up yes-no asker */
    if (interactive)
    {
      log << debug << "mode: setting up yes-no question." << endl;
      rev_yes_no = Gtk::manage (new Gtk::Revealer ());
      rev_yes_no->set_transition_type (Gtk::REVEALER_TRANSITION_TYPE_SLIDE_UP);

      Gtk::Box * rh = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));

      label_yes_no = Gtk::manage (new Gtk::Label ());
      rh->pack_start (*label_yes_no, true, true, 5);

      /* buttons */
      Gtk::Button * yes = Gtk::manage (new Gtk::Button("_Yes"));
      Gtk::Button * no  = Gtk::manage (new Gtk::Button("_No"));

      rh->pack_end (*yes, false, true, 5);
      rh->pack_end (*no, false, true, 5);

      pack_end (*rev_yes_no, false, true, 5);

      rev_yes_no->add (*rh);
      rev_yes_no->set_reveal_child (false);
    }
  }

  void Mode::ask_yes_no (
      ustring question,
      function <void (bool)> closure)
  {
    if (!interactive) throw logic_error ("mode is not interactive!");

    log << info << "mode: " << question << endl;
    rev_yes_no->set_reveal_child (true);
    label_yes_no->set_text (question);
  }

  Mode::~Mode () {
  }

}

