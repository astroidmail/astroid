# include <iostream>
# include <boost/algorithm/string/replace.hpp>

# include "astroid.hh"
# include "db.hh"
# include "edit_message.hh"
# include "reply_message.hh"

# include "actions/action_manager.hh"
# include "actions/onmessage.hh"

# include "message_thread.hh"
# include "utils/address.hh"

using namespace std;

namespace Astroid {
  ReplyMessage::ReplyMessage (MainWindow * mw, refptr<Message> _msg, ReplyMode rmode)
    : EditMessage (mw, false)
  {
    msg = _msg;

    LOG (info) << "re: reply to: " << msg->mid;

    mailinglist_reply_to_sender = astroid->config ().get<bool> ("mail.reply.mailinglist_reply_to_sender");

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

    ustring quoting_a = ustring::compose (astroid->config ().get<string> ("mail.reply.quote_line"),
        boost::replace_all_copy (string(Address(msg->sender.raw()).fail_safe_name()), "%", "%%"),
        boost::replace_all_copy (string(msg->pretty_verbose_date()), "%", "%%"));


    /* quote string can also be formatted using date formats:
     * https://developer.gnome.org/glibmm/stable/classGlib_1_1DateTime.html#a820ed73fbf469a24f86f417540873339
     *
     * note that the format specifiers in the docs use a leading '\' this should
     * be a leading '%'.
     *
     */
    Glib::DateTime dt = Glib::DateTime::create_now_local (msg->time);
    quoting_a = dt.format (quoting_a);

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

    if (msg->is_list_post ()) {
      row = *(reply_store->append());
      row[reply_columns.reply_string] = "Mailinglist";
      row[reply_columns.reply] = Rep_MailingList;
    } else {
      if (rmode == Rep_MailingList) {
        LOG (warn) << "re: message is not a list post, using default reply to all.";
        rmode = Rep_All;
      }
    }

    reply_mode_combo->set_active (rmode); // must match order in ReplyMode
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

    /* match encryption / signing to original message if possible */
    {
      Account a = accounts->accounts[account_no];
      if (a.has_gpg) {
        if (msg->is_encrypted ()) {

          switch_encrypt->set_active (true);
          switch_sign->set_active (true);

        } else if (msg->is_signed ()) {

          switch_sign->set_active (true);

        }
      }
    }

    editor->start_editor_when_ready = true;

    /* sent signal */
    message_sent_attempt().connect (
        sigc::mem_fun (this, &ReplyMessage::on_message_sent_attempt_received));

    keys.title = "Reply mode";

    keys.register_key ("r", "reply.cycle_reply_to",
        "Cycle through reply selector",
        [&] (Key) {
          if (editor->started ()) {
            set_warning ("Cannot change reply to when editing.");

            return true;
          }

          /* cycle through reply combo box */
          if (!message_sent && !sending_in_progress.load()) {
            int i = reply_mode_combo->get_active_row_number ();

            if (i >= (static_cast<int>(reply_store->children().size())-1))
              i = 0;
            else
              i++;

            reply_mode_combo->set_active (i);
          }

          return true;
        });

    keys.register_key ("R", "reply.open_reply_to",
        "Open reply selector",
        [&] (Key) {

          if (editor->started ()) {
            set_warning ("Cannot change reply to when editing.");

            return true;
          }

          /* bring up reply combo box */
          if (!message_sent && !sending_in_progress.load()) {
            reply_mode_combo->popup ();
          }

          return true;
        });

    edit_when_ready ();
  }

  void ReplyMessage::on_receiver_combo_changed () {
    if (!in_read) {
      load_receivers ();

      prepare_message ();
      read_edited_message ();
    }
  }

  void ReplyMessage::load_receivers () {
    auto iter = reply_mode_combo->get_active ();
    auto row = *iter;
    ReplyMode rmode = row[reply_columns.reply];

    if (rmode == Rep_Sender || rmode == Rep_Default) {

      if (rmode == Rep_Default && msg->reply_to.length () > 0) {
        to = msg->reply_to;
      } else {
        /* no reply-to or Rep_Sender */
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
      al.remove_duplicates ();

      to = al.str ();

      AddressList ac (msg->cc ());
      ac.remove_me ();
      ac.remove_duplicates ();
      ac -= al;
      cc = ac.str ();

      AddressList acc (msg->bcc ());
      acc.remove_me ();
      acc.remove_duplicates ();
      acc -= al;
      acc -= ac;
      bcc = acc.str ();

    } else if (rmode == Rep_MailingList) {
      AddressList al = msg->list_post ();
      al += msg->to();

      if (mailinglist_reply_to_sender) {
        ustring from;
        if (msg->reply_to.length () > 0) {
          from = msg->reply_to;
        } else {
          from = msg->sender;
        }
        al += Address(from);
      }

      al.remove_me ();
      al.remove_duplicates ();
      to = al.str ();

      AddressList ac (msg->cc ());
      ac -= al;
      ac.remove_me ();
      ac.remove_duplicates ();
      cc = ac.str ();

      AddressList acc (msg->bcc ());
      acc -= al;
      acc -= ac;
      acc.remove_me ();
      acc.remove_duplicates ();
      bcc = acc.str ();
    }
  }

  void ReplyMessage::on_message_sent_attempt_received (bool res) {
    if (res) {
      LOG (info) << "re: message successfully sent, adding replied tag to original.";

      if (!msg->in_notmuch) {
        LOG (warn) << "re: message not in notmuch.";
        return;
      }

      astroid->actions->doit (refptr<Action>(
            new OnMessageAction (msg->mid, msg->tid,

              [] (Db * db, notmuch_message_t * msg) {

                notmuch_message_add_tag (msg, "replied");

                if (db->maildir_synchronize_flags){
                  notmuch_message_tags_to_maildir_flags (msg);
                }

              })));
    }
  }
}

