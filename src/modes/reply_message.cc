# include <iostream>

# include "astroid.hh"
# include "log.hh"
# include "edit_message.hh"
# include "reply_message.hh"

# include "message_thread.hh"
# include "utils/address.hh"

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
      set_label ("New message: " + subject);
    }

    /* quote original message */
    ostringstream quoted;

    ustring quoting_a = ustring::compose ("Excerpts from %1's message of %2:",
        Address(msg->sender.raw()).fail_safe_name(), msg->pretty_verbose_date());

    quoted  << quoting_a.raw ()
            << endl;

    string vt = msg->viewable_text(false);
    stringstream sstr (vt);
    while (sstr.good()) {
      string line;
      getline (sstr, line);
      quoted << ">";

      if (line[0] != '>')
        quoted << " ";

      quoted << line << endl;
    }

    body = ustring(quoted.str());

    if (msg->reply_to.length () > 0) {
      to = msg->reply_to;
    } else {
      to = msg->sender;
    }

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
    row[reply_columns.reply_string] = "Default (Reply-to address)";
    row[reply_columns.reply] = Rep_Default;
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
    reply_mode_combo->pack_start (reply_columns.reply_string);

    /* reload message */
    prepare_message ();
    read_edited_message ();

    start_vim_on_socket_ready = true;
  }
}

