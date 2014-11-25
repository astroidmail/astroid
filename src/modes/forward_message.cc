# include <iostream>
# include <memory>

# include "astroid.hh"
# include "log.hh"
# include "edit_message.hh"
# include "forward_message.hh"

# include "message_thread.hh"
# include "utils/address.hh"
# include "chunk.hh"

using namespace std;

namespace Astroid {
  ForwardMessage::ForwardMessage (MainWindow * mw, refptr<Message> _msg) : EditMessage (mw) {
    msg = _msg;

    log << info << "fwd: forwarding message " << msg->mid << endl;

    /* set subject */
    subject = ustring::compose ("Fwd: %1", msg->subject);

    if (subject.size () > 0) {
      set_label ("New message: " + subject);
    }

    /* quote original message */
    ostringstream quoted;

    ustring quoting_a = ustring::compose ("Forwarding %1's message of %2:",
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


    /* try to figure which account the message was sent to, using
     * first match. */
    for (Address &a : msg->all_to_from().addresses) {
      if (accounts->is_me (a)) {
        set_from_to (a);
        break;
      }
    }

    /* reload message */
    prepare_message ();
    read_edited_message ();

    activate_field (Thread);
  }
}

