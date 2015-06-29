# pragma once

# include <vector>
# include <ctime>
# include <atomic>
# include <memory>

# include <gtkmm/socket.h>
# include <glibmm/iochannel.h>

# include <boost/filesystem.hpp>

# include "proto.hh"
# include "astroid.hh"
# include "config.hh"
# include "mode.hh"
# include "compose_message.hh"
# include "account_manager.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  class EditMessage : public Mode {
    public:
      EditMessage (MainWindow *);
      EditMessage (MainWindow *, ustring to);
      ~EditMessage ();

      Gtk::Box * box_message;

      enum Field {
        From = 0,
        Encryption,
        Editor,
        Thread,
        no_fields // last
      };

      Gtk::ComboBox *from_combo, *encryption_combo,
        *reply_mode_combo;
      Gtk::Revealer *fields_revealer;
      Gtk::Revealer *reply_revealer;
      Gtk::Revealer *encryption_revealer;

      Field current_field = From;
      void activate_field (Field);

      ustring msg_id;

      ustring to;
      ustring cc;
      ustring bcc;
      ustring subject;
      ustring body;

      ustring references;
      ustring inreplyto;

      vector<shared_ptr<ComposeMessage::Attachment>> attachments;
      void add_attachment (ComposeMessage::Attachment *);
      void attach_file ();

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

      void set_from (Account *);
      void set_from (Address);

      /* encryption settings */
      enum Encryption {
        Enc_None,
        Enc_Sign,
        Enc_Encrypt,
        Enc_SignAndEncrypt,
      };

      class EncryptionColumns : public Gtk::TreeModel::ColumnRecord {
        public:
          EncryptionColumns () { add (encryption_string); add (encryption); }

          Gtk::TreeModelColumn<ustring> encryption_string;
          Gtk::TreeModelColumn<int> encryption;
      };

      EncryptionColumns enc_columns;
      refptr<Gtk::ListStore> enc_store;

      bool check_fields ();
      bool send_message ();
      ComposeMessage * make_message ();

      ComposeMessage * sending_message;
      atomic<bool> sending_in_progress;
      void send_message_finished (bool result);


      void prepare_message ();

    protected:
      ptree editor_config;

      Gtk::Box *    editor_box;
      Gtk::Socket * editor_socket;
      ThreadView *  thread_view;
      Gtk::Revealer *editor_rev, *thread_rev;
      void plug_added ();
      bool plug_removed ();
      void socket_realized ();
      bool socket_ready = false;
      bool vim_started  = false;
      bool vim_ready    = false;
      bool start_vim_on_socket_ready = false;

      static  int edit_id; // must be incremented each time a new editor is started
      int     id;          // id of this instance
      time_t  msg_time;
      ustring vim_server;

      void editor_toggle (bool); // enable or disable editor or
                                 // thread view
      void fields_show ();       // show fields
      void fields_hide ();       // hide fields
      void read_edited_message (); // load data from message after
                                   // it has been edited.
      void on_tv_ready ();
      ustring warning_str;
      ustring info_str;

      void vim_start ();
      void vim_stop ();
      void vim_remote_expr (ustring);
      void vim_remote_keys (ustring);
      void vim_remote_files (ustring);

      void reset_entry (Gtk::Entry *);

      /* gvim config */
      string gvim_cmd;
      string gvim_args;

      AccountManager * accounts;

      path tmpfile_path;
      fstream tmpfile;
      void make_tmpfile ();

      Gtk::Image message_sending_status_icon;
      bool status_icon_visible = false;


      bool message_sent = false;
      void lock_message_after_send ();

      virtual ModeHelpInfo * key_help () override;

    private:
      void on_from_combo_changed ();
      //bool on_from_combo_key_press (GdkEventKey *);
      void on_element_action (int id, char action);

    public:
      void grab_modal () override;
      void release_modal () override;

      typedef sigc::signal <void, bool> type_message_sent_attempt;
      type_message_sent_attempt message_sent_attempt ();
      void emit_message_sent_attempt (bool);

    protected:
      type_message_sent_attempt m_message_sent_attempt;

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

