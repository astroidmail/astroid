# pragma once

# include "astroid.hh"
# include "proto.hh"
# include "edit_message.hh"

namespace Astroid {
  class ForwardMessage : public EditMessage {
    public:
      typedef enum {
        FwdDefault,
        FwdInline,
        FwdAttach,
      } FwdDisposition;

      ForwardMessage (MainWindow *, refptr<Message>, FwdDisposition disp = FwdDefault);

      refptr<Message> msg;

    private:
      void on_message_sent_attempt_received (bool);
  };
}

