# include <iostream>
# include <memory>
# include <boost/algorithm/string/replace.hpp>

# include "astroid.hh"
# include "db.hh"
# include "edit_message.hh"
# include "forward_message.hh"

# include "actions/action_manager.hh"
# include "actions/onmessage.hh"

# include "message_thread.hh"
# include "utils/address.hh"
# include "chunk.hh"

namespace Astroid {
  ForwardMessage::ForwardMessage (MainWindow * mw,
      refptr<Message> _msg,
      FwdDisposition disp) : EditMessage (mw, false)
  {
    using std::endl;
    using std::string;

    msg = _msg;

    LOG (info) << "fwd: forwarding message " << msg->mid;

    /* set subject */
    subject = ustring::compose ("Fwd: %1", msg->subject);

    if (subject.size () > 0) {
      set_label ("New message: " + subject);
    }

    if (disp == FwdDefault) {
      disp = (astroid->config().get<string> ("mail.forward.disposition") == "attachment") ? FwdAttach : FwdInline;
    }

    if (disp == FwdAttach) {

      add_attachment (new ComposeMessage::Attachment (msg));

    } else {

      /* quote original message */
      std::ostringstream quoted;

      ustring quoting_a = ustring::compose (astroid->config ().get<string> ("mail.forward.quote_line"),
          boost::replace_all_copy (string(Address(msg->sender.raw()).fail_safe_name()), "%", "%%"),
          boost::replace_all_copy (string(msg->pretty_verbose_date()), "%", "%%"));

      /* date format */
      Glib::DateTime dt = Glib::DateTime::create_now_local (msg->time);
      quoting_a = dt.format (quoting_a);

      quoted  << quoting_a.raw ()
              << endl;

      /* add forward header */
      quoted << "From: " << msg->sender << endl;
      quoted << "Date: " << msg->pretty_verbose_date() << endl;
      quoted << "Subject: " << msg->subject << endl;
      quoted << "To: " << AddressList(msg->to()).str () << endl;
      auto cc = AddressList (msg->cc());
      if (cc.addresses.size () > 0)
        quoted << "Cc: " << AddressList(msg->cc()).str () << endl;
      quoted << endl;

      string vt = msg->viewable_text(false);
      quoted << vt;

      body = ustring(quoted.str());

      for (auto &c : msg->attachments ()) {
        add_attachment (new ComposeMessage::Attachment (c));
      }

      /* TODO: add non-text parts */
    }


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

    /* sent signal */
    message_sent_attempt().connect (
        sigc::mem_fun (this, &ForwardMessage::on_message_sent_attempt_received));

    keys.title = "Forward mode";

    edit_when_ready ();
  }

  void ForwardMessage::on_message_sent_attempt_received (bool res) {
    using std::endl;
    if (res) {
      LOG (info) << "fwd: message successfully sent, adding passed tag to original.";

      if (!msg->in_notmuch) {
        LOG (warn) << "fwd: message not in notmuch.";
        return;
      }

      astroid->actions->doit (refptr<Action>(
            new OnMessageAction (msg->mid, msg->tid,

              [] (Db * db, notmuch_message_t * msg) {

                notmuch_message_add_tag (msg, "passed");

                if (db->maildir_synchronize_flags){
                  notmuch_message_tags_to_maildir_flags (msg);
                }

              })));
    }
  }
}

