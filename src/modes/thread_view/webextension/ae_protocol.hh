# pragma once

# include <giomm.h>
# include "messages.pb.h"

namespace Astroid {
  class AeProtocol {
    public:
      typedef enum _MessageTypes {
        Debug = 0,
        StyleSheet,
        State,
        Focus,
        Mark,
        Hidden,
        AddMessage,
        UpdateMessage,
      } MessageTypes;

      static void send_message (MessageTypes mt, const ::google::protobuf::Message &m, Glib::RefPtr<Gio::OutputStream> ostream);
  };
}



