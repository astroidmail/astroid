# pragma once

# include <vector>
# include <ctime>
# include <atomic>
# include <memory>
# include <fstream>
# include <mutex>

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
# ifndef DISABLE_EMBEDDED
    friend Plugin;
# endif
    friend External;

    public:
      EditMessage (MainWindow *, bool edit_when_ready = true);
      EditMessage (MainWindow *, ustring to, ustring from = "");
      EditMessage (MainWindow *, refptr<Message> _msg);
      ~EditMessage ();

    protected:
      void edit_when_ready ();

    public:
      Gtk::Box * box_message;

      Gtk::ComboBox *from_combo, *reply_mode_combo;
      Gtk::Switch   *switch_signature;
      Gtk::Switch   *switch_encrypt;
      Gtk::Switch   *switch_sign;
      Gtk::Revealer *fields_revealer;
      Gtk::Revealer *reply_revealer;
      Gtk::Revealer *encryption_revealer;

# ifndef DISABLE_EMBEDDED
      bool embed_editor = true;
# else
      const bool embed_editor = false;
# endif

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

      bool set_from (Account *);
      bool set_from (Address);

      bool check_fields ();
      std::vector<ustring> attachment_words = { "attach" }; // defined in config

      bool send_message ();
      ComposeMessage * setup_message ();
      void             finalize_message (ComposeMessage *);
      ComposeMessage * make_message ();

      ComposeMessage * sending_message;
      std::atomic<bool> sending_in_progress;
      void send_message_finished (bool result);
      void update_send_message_status (bool warn, ustring msg);

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
      std::mutex message_draft_m;  // locks message draft
      std::atomic<bool> in_read;   // true if we are already in read
      void on_tv_ready ();
      void set_warning (ustring);
      void set_info (ustring);

      ustring warning_str = "";
      ustring info_str = "";

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

