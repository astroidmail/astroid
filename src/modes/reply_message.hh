# pragma once

# include <iostream>

# include "astroid.hh"
# include "proto.hh"
# include "edit_message.hh"

using namespace std;

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

      virtual ModeHelpInfo * key_help ();

    private:
      void load_receivers ();
      void on_receiver_combo_changed ();

    protected:
      virtual bool on_key_press_event (GdkEventKey *) override;
  };
}

