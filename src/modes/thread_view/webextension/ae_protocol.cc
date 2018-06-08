# include "ae_protocol.hh"
# include "messages.pb.h"

# include <giomm.h>
# include <string>
# include <mutex>

namespace Astroid {

  void AeProtocol::send_message (MessageTypes mt, const ::google::protobuf::Message &m, Glib::RefPtr<Gio::OutputStream> ostream, std::mutex &m_ostream) {

    std::lock_guard<std::mutex> lk (m_ostream);

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

