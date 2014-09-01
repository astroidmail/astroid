# include <iostream>

# include "astroid.hh"
# include "log.hh"
# include "edit_message.hh"
# include "reply_message.hh"

# include "message_thread.hh"

using namespace std;

namespace Astroid {
  ReplyMessage::ReplyMessage (MainWindow * mw, refptr<Message> _msg) : EditMessage (mw) {
    msg = _msg;

    log << info << "re: reply to: " << msg->mid << endl;

    /* set subject */
    if (!(msg->subject.find_first_of ("Re:") == 0)) {
      subject = ustring::compose ("Re: %1", msg->subject);
    } else {
      subject = msg->subject;
    }

    if (subject.size () > 0) {
      ((Gtk::Label*)tab_widget)->set_text ("New message: " + subject);
    }

    /* quote original message */
    ostringstream quoted;

    ustring quoting_a = ustring::compose ("%1 wrote on %2:",
        msg->sender.raw(), msg->date());

    quoted  << quoting_a.raw ()
            << endl;

    string vt = msg->viewable_text(false);
    stringstream sstr (vt);
    while (sstr.good()) {
      string line;
      getline (sstr, line);
      quoted << "> " << line << endl;
    }

    body = ustring(quoted.str());

    to = msg->sender;

    references = msg->references + " <" + msg->mid + ">";
    inreplyto  = "<" + msg->mid + ">";

    /* reply mode combobox */
    reply_revealer->set_reveal_child (true);

    reply_store = Gtk::ListStore::create (reply_columns);
    reply_mode_combo->set_model (reply_store);

    auto row = *(reply_store->append());
    row[reply_columns.reply_string] = "Custom";
    row[reply_columns.reply] = Rep_Custom;
    row = *(reply_store->append());
    row[reply_columns.reply_string] = "Sender";
    row[reply_columns.reply] = Rep_Sender;
    row = *(reply_store->append());
    row[reply_columns.reply_string] = "All";
    row[reply_columns.reply] = Rep_All;
    row = *(reply_store->append());
    row[reply_columns.reply_string] = "Mailinglist";
    row[reply_columns.reply] = Rep_MailingList;

    reply_mode_combo->set_active (0);
    reply_mode_combo->pack_start (enc_columns.encryption_string);

    editor_toggle (true);
    activate_field (Editor);
  }
}

