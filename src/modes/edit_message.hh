# pragma once

# include <vector>
# include <ctime>

# include <gtkmm/socket.h>
# include <glibmm/iochannel.h>

# include <boost/filesystem.hpp>

# include "proto.hh"
# include "astroid.hh"
# include "config.hh"
# include "mode.hh"
# include "contacts.hh"
# include "account_manager.hh"

using namespace std;
using namespace boost::filesystem;

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

      Gtk::Entry *to, *cc, *bcc, *subject;
      Gtk::ComboBox *from_combo;

      Contacts * contacts;
      refptr<Contacts::ContactCompletion> contact_completion;

      Gtk::Image *editor_img;
      vector<Gtk::Entry *> fields;

      Field current_field = From;
      void activate_field (Field);

      ustring msg_id;

      /* from combobox */
      class FromColumns : public Gtk::TreeModel::ColumnRecord {
        public:
          FromColumns () { add (name_and_address); add (account); }
          Gtk::TreeModelColumn<ustring> name_and_address;
          Gtk::TreeModelColumn<Account *> account;
      };

      FromColumns from_columns;
      refptr<Gtk::ListStore> from_store;
      int account_no;

      bool check_fields ();
      void send_message ();

    private:
      ptree editor_config;

      Gtk::Box *editor_box;
      Gtk::Socket *editor_socket;
      Gtk::TextView *text_view;
      void plug_added ();
      bool plug_removed ();
      void socket_realized ();
      bool vim_started = false;

      static  int edit_id; // must be incremented each time a new editor is started
      int     id;          // id of this instance
      time_t  msg_time;
      ustring vim_server;

      void editor_toggle (bool);
      void vim_start ();
      void vim_stop ();
      void vim_remote_expr (ustring);
      void vim_remote_keys (ustring);
      void vim_remote_files (ustring);

      void reset_fields ();
      void reset_entry (Gtk::Entry *);

      bool in_edit = false;
      bool editor_active = false;
      int  esc_count = 0;

      /* gvim config */
      bool double_esc_deactivates = true; // set in user config
      bool send_default = true;
      string default_cmd_on_enter;
      string gvim_cmd;
      string gvim_args;

      AccountManager * accounts;

      path tmpfile_path;
      fstream tmpfile;
      void make_tmpfile ();

    public:
      void grab_modal () override;
      void release_modal () override;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

