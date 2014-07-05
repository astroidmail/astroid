# include <iostream>

# include "astroid.hh"
# include "edit_message.hh"
# include "reply_message.hh"

# include "message_thread.hh"

using namespace std;

namespace Astroid {
  ReplyMessage::ReplyMessage (refptr<Message> _msg) : EditMessage () {
    msg = _msg;

    cout << "re: reply to: " << msg->mid << endl;

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
    tmpfile.open (tmpfile_path.c_str(), fstream::out);

    ustring quoting_a = ustring::compose ("%1 wrote on %2:", msg->sender.raw(), msg->date());

    tmpfile << quoting_a.raw ()
            << endl;

    string vt = msg->viewable_text(false);
    stringstream sstr (vt);
    while (sstr.good()) {
      string line;
      getline (sstr, line);
      tmpfile << "> " << line << endl;
    }
    tmpfile.close ();

    to = msg->sender;

    activate_field (Editor);
  }
}

