# pragma once

# include <vector>
# include <ctime>

# include <gtkmm/socket.h>
# include <glibmm/iochannel.h>

# include "astroid.hh"
# include "config.hh"
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

      ustring msg_id;

    private:
      ptree editor_config;

      Gtk::Box *editor_box;
      Gtk::Socket *editor_socket;
      void plug_added ();
      bool plug_removed ();
      void socket_realized ();
      bool vim_started = false;

      static  int edit_id; // must be incremented each time a new editor is started
      int     id;          // id of this instance
      time_t  msg_time;
      ustring vim_server;

      void reset_fields ();
      void reset_entry (Gtk::Entry *);

      bool in_edit = false;
      bool editor_active = false;
      bool double_esc_deactivates = true; // set in user config
      int  esc_count = 0;

      AccountManager * accounts;

    public:
      void grab_modal () override;
      void release_modal () override;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

