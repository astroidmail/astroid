# pragma once

# include <vector>
# include <ctime>
# include <atomic>
# include <memory>
# include <fstream>

# include <gtkmm/socket.h>
# include <glibmm/iochannel.h>

# include <boost/filesystem.hpp>

# include "proto.hh"
# include "astroid.hh"
# include "config.hh"
# include "mode.hh"
# include "editor/editor.hh"
# include "editor/plugin.hh"
# include "editor/external.hh"
# include "compose_message.hh"
# include "account_manager.hh"
# include "thread_view/thread_view.hh"

namespace Astroid {
  class EditMessage : public Mode {
    friend Editor;
    friend Plugin;
    friend External;

    public:
      EditMessage (MainWindow *);
      EditMessage (MainWindow *, ustring to, ustring from = "");
      EditMessage (MainWindow *, refptr<Message> _msg);
      ~EditMessage ();

      Gtk::Box * box_message;

      Gtk::ComboBox *from_combo, *reply_mode_combo;
      Gtk::Switch   *switch_signature;
      Gtk::Switch   *switch_encrypt;
      Gtk::Switch   *switch_sign;
      Gtk::Revealer *fields_revealer;
      Gtk::Revealer *reply_revealer;
      Gtk::Revealer *encryption_revealer;

      bool embed_editor = true;

      Editor * editor;
      bool editor_active = false;
      void activate_editor ();

      void switch_signature_set ();
      void reset_signature ();

      ustring msg_id;

      ustring to;
      ustring cc;
      ustring bcc;
      ustring subject;
      ustring body;

      ustring references;
      ustring inreplyto;

      std::vector<std::shared_ptr<ComposeMessage::Attachment>> attachments;
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

      bool check_fields ();
      bool send_message ();
      ComposeMessage * make_message ();

      ComposeMessage * sending_message;
      std::atomic<bool> sending_in_progress;
      void send_message_finished (bool result);

      /* make a draft message that can be edited */
      void prepare_message ();

      /* draft */
      bool save_draft_on_force_quit;
      bool save_draft ();
      void delete_draft ();
      static void delete_draft (refptr<Message> draft_msg);
      refptr<Message> draft_msg;
      bool draft_saved = false;

    protected:
      ptree editor_config;

      Gtk::Box *    editor_box;
      ThreadView *  thread_view;
      Gtk::Revealer *editor_rev, *thread_rev;

      static  int edit_id; // must be incremented each time a new editor is started
      int     id;          // id of this instance
      time_t  msg_time;

      void editor_toggle (bool); // enable or disable editor or thread view

      void fields_show ();       // show fields
      void fields_hide ();       // hide fields
      void read_edited_message (); // load data from message after
                                   // it has been edited.
      void on_tv_ready ();
      ustring warning_str;
      ustring info_str;

      AccountManager * accounts;

      boost::filesystem::path tmpfile_path;
      std::fstream tmpfile;
      void make_tmpfile ();

      Gtk::Image message_sending_status_icon;
      bool status_icon_visible = false;


      bool message_sent = false;
      void lock_message_after_send ();

    private:
      void on_from_combo_changed ();
      //bool on_from_combo_key_press (GdkEventKey *);
      void on_element_action (int id, ThreadView::ElementAction action);

    public:
      void grab_modal () override;
      void release_modal () override;
      void close (bool = false) override;

      typedef sigc::signal <void, bool> type_message_sent_attempt;
      type_message_sent_attempt message_sent_attempt ();
      void emit_message_sent_attempt (bool);

    protected:
      type_message_sent_attempt m_message_sent_attempt;
  };
}

