# include "ae_protocol.hh"
# include "messages.pb.h"

# include <giomm.h>
# include <string>

namespace Astroid {

  void AeProtocol::send_message (MessageTypes mt, ::google::protobuf::Message &m, Glib::RefPtr<Gio::OutputStream> ostream) {

    std::string o;

    m.SerializeToString (&o);

    /* send size of message */
    gsize sz = o.size ();
    if (sz == 0) {
      return;
    }
    ostream->write ((char*)&sz, sizeof(sz));

    /* send message type */
    ostream->write ((char*) &mt, sizeof (mt));

    /* send message */
    ostream->write (o);
    ostream->flush ();
  }

}

