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
      subject->set_text (ustring::compose ("Re: %1", msg->subject));
    } else {
      subject->set_text (msg->subject);
    }

    

  }
}

