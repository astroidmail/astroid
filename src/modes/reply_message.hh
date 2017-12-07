# pragma once

# include "proto.hh"
# include "edit_message.hh"

namespace Astroid {
  class ReplyMessage : public EditMessage {
    public:
      /* reply settings */
      enum ReplyMode {
        Rep_Custom,
        Rep_Default, /* reply-to */
        Rep_Sender,
        Rep_All,
        Rep_MailingList,
      };

      ReplyMessage (MainWindow *, refptr<Message>, ReplyMode rmode = Rep_Default);

      refptr<Message> msg;

      class ReplyModeColumns : public Gtk::TreeModel::ColumnRecord {
        public:
          ReplyModeColumns () { add (reply_string); add (reply); }

          Gtk::TreeModelColumn<ustring> reply_string;
          Gtk::TreeModelColumn<ReplyMode> reply;
      };

      ReplyModeColumns reply_columns;
      refptr<Gtk::ListStore> reply_store;

    private:
      void load_receivers ();
      void on_receiver_combo_changed ();

      void on_message_sent_attempt_received (bool);

      bool mailinglist_reply_to_sender;
  };
}

