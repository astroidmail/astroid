# include <iostream>
# include <memory>

# include "astroid.hh"
# include "db.hh"
# include "log.hh"
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
      FwdDisposition disp) : EditMessage (mw)
  {
    using std::endl;
    using std::string;

    msg = _msg;

    log << info << "fwd: forwarding message " << msg->mid << endl;

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
          Address(msg->sender.raw()).fail_safe_name(), msg->pretty_verbose_date());

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

    activate_field (Thread);

    /* sent signal */
    message_sent_attempt().connect (
        sigc::mem_fun (this, &ForwardMessage::on_message_sent_attempt_received));

    keys.title = "Forward mode";
  }

  void ForwardMessage::on_message_sent_attempt_received (bool res) {
    using std::endl;
    if (res) {
      log << info << "fwd: message successfully sent, adding passed tag to original." << endl;

      if (!msg->in_notmuch) {
        log << warn << "fwd: message not in notmuch." << endl;
        return;
      }

      astroid->actions->doit (refptr<Action>(
            new OnMessageAction (msg->mid,

              [] (Db *, notmuch_message_t * msg) {

                notmuch_message_add_tag (msg, "passed");

              })));
    }
  }
}

