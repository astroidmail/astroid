# include <iostream>

# include "message.hh"

using namespace std;

namespace Gulp {
  /* --------
   * Message
   * --------
   */
  Message::Message () { }
  Message::Message (ustring _fname) : fname (_fname) {
  }

  void Message::load_message () {
  }


  /* --------
   * MessageThread
   * --------
   */

  MessageThread::MessageThread () { }
  MessageThread::MessageThread (ustring _tid) : threadid (_tid) {

  }

  void MessageThread::load_messages () {
  }

  void MessageThread::reload_messages () {
  }

}

