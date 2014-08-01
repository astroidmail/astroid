# pragma once

# include <iostream>

# include "astroid.hh"
# include "proto.hh"
# include "edit_message.hh"

using namespace std;

namespace Astroid {
  class ReplyMessage : public EditMessage {
    public:
      ReplyMessage (MainWindow *, refptr<Message>);

      refptr<Message> msg;

  };
}

