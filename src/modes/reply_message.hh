# pragma once

# include <iostream>

# include "astroid.hh"
# include "proto.hh"
# include "edit_message.hh"

using namespace std;

namespace Astroid {
  class ReplyMessage : public EditMessage {
    public:
      ReplyMessage (MainWindow *, refptr<Message>);

      refptr<Message> msg;

      /* reply settings */
      enum ReplyMode {
        Rep_Custom,
        Rep_Sender,
        Rep_All,
        Rep_MailingList,
      };

      class ReplyModeColumns : public Gtk::TreeModel::ColumnRecord {
        public:
          ReplyModeColumns () { add (reply_string); add (reply); }

          Gtk::TreeModelColumn<ustring> reply_string;
          Gtk::TreeModelColumn<ReplyMode> reply;
      };

      ReplyModeColumns reply_columns;
      refptr<Gtk::ListStore> reply_store;
  };
}

