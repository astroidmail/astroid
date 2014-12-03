# include <iostream>

# include "astroid.hh"
# include "log.hh"
# include "edit_message.hh"
# include "reply_message.hh"

# include "message_thread.hh"
# include "utils/address.hh"

using namespace std;

namespace Astroid {
  ReplyMessage::ReplyMessage (MainWindow * mw, refptr<Message> _msg, ReplyMode rmode)
    : EditMessage (mw)
  {
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

    reply_mode_combo->set_active (rmode); // must match order
    reply_mode_combo->pack_start (reply_columns.reply_string);

    load_receivers ();

    reply_mode_combo->signal_changed().connect (
        sigc::mem_fun (this, &ReplyMessage::on_receiver_combo_changed));

    /* try to figure which account the message was sent to, using
     * first match. */
    for (Address &a : msg->all_to_from().addresses) {
      if (accounts->is_me (a)) {
        set_from (a);
        break;
      }
    }

    /* reload message */
    prepare_message ();
    read_edited_message ();

    start_vim_on_socket_ready = true;
  }

  void ReplyMessage::on_receiver_combo_changed () {
    load_receivers ();

    prepare_message ();
    read_edited_message ();
  }

  void ReplyMessage::load_receivers () {
    auto iter = reply_mode_combo->get_active ();
    auto row = *iter;
    ReplyMode rmode = row[reply_columns.reply];

    if (rmode == Rep_Sender || rmode == Rep_Default) {

      if (msg->reply_to.length () > 0) {
        to = msg->reply_to;
      } else {
        to = msg->sender;
      }

      auto msg_to = Address(to);
      if (accounts->is_me(msg_to)) {
        AddressList al (msg->to());

        to = al.str ();
      }

      cc = "";
      bcc = "";

    } else if (rmode == Rep_All) {
      ustring from;
      if (msg->reply_to.length () > 0) {
        from = msg->reply_to;
      } else {
        from = msg->sender;
      }

      AddressList al;

      auto msg_from = Address(from);
      if (!accounts->is_me(msg_from)) {
        al += msg_from;
      }

      al += AddressList (msg->to());

      al.remove_me ();

      to = al.str ();

      AddressList ac (msg->cc ());
      ac.remove_me ();
      cc = ac.str ();

      AddressList acc (msg->bcc ());
      acc.remove_me ();
      bcc = acc.str ();
    }
  }

  bool ReplyMessage::on_key_press_event (GdkEventKey * event) {
    log << debug << "re: got key press" << endl;

    if (mode_key_handler (event)) return true;

    switch (event->keyval) {
      case GDK_KEY_r:
        {
          /* cycle through reply combo box */
          if (!message_sent && !sending_in_progress.load()) {
            int i = reply_mode_combo->get_active_row_number ();
            if (i >= (static_cast<int>(reply_store->children().size())-1)) i = 0;
            else i++;
            reply_mode_combo->set_active (i);
          }

          return true;
        }
      case GDK_KEY_R:
        {
          /* bring up reply combo box */
          if (!message_sent && !sending_in_progress.load()) {
            reply_mode_combo->popup ();
          }

          return true;
        }
    }

    return EditMessage::on_key_press_event (event);
  }

  ModeHelpInfo * ReplyMessage::key_help () {
    ModeHelpInfo * m = new ModeHelpInfo ();

    m->parent   = EditMessage::key_help ();
    m->toplevel = false;
    m->title    = "Reply message";

    m->keys = {
      { "r", "Cycle through Reply selector" },
      { "R", "Open Reply selecetor" }
    };

    return m;
  }
}

