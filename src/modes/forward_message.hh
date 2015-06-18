# pragma once

# include <iostream>

# include "astroid.hh"
# include "proto.hh"
# include "edit_message.hh"

using namespace std;

namespace Astroid {
  class ForwardMessage : public EditMessage {
    public:
      ForwardMessage (MainWindow *, refptr<Message>);

      refptr<Message> msg;

    private:
      void on_message_sent_attempt_received (bool);
  };
}

