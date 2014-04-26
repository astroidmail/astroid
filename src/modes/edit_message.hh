# pragma once

# include <vector>

# include "astroid.hh"
# include "mode.hh"

using namespace std;

namespace Astroid {
  class EditMessage : public Mode {
    public:
      EditMessage ();

      Gtk::Box * box_message;

      enum Field {
        From = 0,
        To,
        Cc,
        Bcc,
        Message,
      };

      Gtk::Entry * from, *to, *cc, *bcc;
      vector<Gtk::Entry *> fields;

      Field current_field = From;
      void activate_field (Field);

    private:
      void reset_fields ();
      void reset_entry (Gtk::Entry *);

      bool in_edit = false;

    public:
      void grab_modal () override;
      void release_modal () override;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

