# pragma once

# include <vector>
# include <ctime>

# include <gtkmm/socket.h>
# include <glibmm/iochannel.h>

# include "astroid.hh"
# include "mode.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class EditMessage : public Mode {
    public:
      EditMessage ();
      ~EditMessage ();

      Gtk::Box * box_message;

      enum Field {
        From = 0,
        To,
        Cc,
        Bcc,
        Subject,
        Editor,
        no_fields // last
      };

      Gtk::Entry *from, *to, *cc, *bcc, *subject;
      Gtk::Image *editor_img;
      vector<Gtk::Entry *> fields;

      Field current_field = From;
      void activate_field (Field);

    private:
      Gtk::Box *editor_box;
      Gtk::Socket *editor_socket;
      void plug_added ();
      bool plug_removed ();
      void socket_realized ();
      bool vim_started = false;

      time_t  msg_time;
      ustring vim_server;

      void reset_fields ();
      void reset_entry (Gtk::Entry *);

      bool in_edit = false;
      bool editor_active = false;

      AccountManager * accounts;

    public:
      void grab_modal () override;
      void release_modal () override;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

