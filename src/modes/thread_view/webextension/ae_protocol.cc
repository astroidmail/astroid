# include "ae_protocol.hh"
# include "messages.pb.h"

# include <giomm.h>
# include <string>
# include <mutex>
# include <iostream>

#ifdef ASTROID_WEBEXTENSION

# include <boost/log/core.hpp>
# include <boost/log/trivial.hpp>
# define LOG(x) BOOST_LOG_TRIVIAL(x)
# define warn warning

#else

# include "astroid.hh"

#endif

namespace Astroid {

  const char * AeProtocol::MessageTypeStrings[] = {
    "Debug",
    "Ack",
    "Info",
    "Page",
    "State",
    "Indent",
    "AllowRemoteImages",
    "Focus",
    "Navigate",
    "Mark",
    "Hidden",
    "ClearMessages",
    "AddMessage",
    "UpdateMessage",
    "RemoveMessage",
  };


  void AeProtocol::send_message (
      MessageTypes mt,
      const ::google::protobuf::Message &m,
      Glib::RefPtr<Gio::OutputStream> ostream)
  {
    std::string o;

    m.SerializeToString (&o);

    /* send size of message */
    gsize sz = o.size ();
    ostream->write ((char*) &sz, sizeof(sz));

    /* send message type */
    ostream->write ((char*) &mt, sizeof (mt));

    /* send message */
    gsize written = 0;
    ostream->write_all (o, written);
    ostream->flush ();
    LOG (debug) << "ae: wrote: " << written << " of " << o.size () << " bytes.";
  }

  void AeProtocol::send_message_async (
      MessageTypes mt,
      const ::google::protobuf::Message &m,
      Glib::RefPtr<Gio::OutputStream> ostream,
      std::mutex &m_ostream)
  {
    LOG (debug) << "ae: sending: " << MessageTypeStrings[mt];
    LOG (debug) << "ae: send (async) waiting for lock";
    std::lock_guard<std::mutex> lk (m_ostream);
    send_message (mt, m, ostream);
    LOG (debug) << "ae: send (async) message sent.";
  }

  AstroidMessages::Ack AeProtocol::send_message_sync (
      MessageTypes mt,
      const ::google::protobuf::Message &m,
      Glib::RefPtr<Gio::OutputStream> ostream,
      std::mutex & m_ostream,
      Glib::RefPtr<Gio::InputStream> istream,
      std::mutex & m_istream)
  {
    LOG (debug) << "ae: sending: " << MessageTypeStrings[mt];
    LOG (debug) << "ae: send (sync) waiting for lock..";
    std::lock_guard<std::mutex> rlk (m_istream);
    std::lock_guard<std::mutex> wlk (m_ostream);
    LOG (debug) << "ae: send (sync) lock acquired.";

    /* send message */
    send_message (mt, m, ostream);

    /* read response */
    LOG (debug) << "ae: send (sync) waiting for ACK..";
    AstroidMessages::Ack a;
    a.set_success (false);

    {
      gsize read = 0;

      /* read size of message */
      gsize sz;

      try {
        read = istream->read ((char*)&sz, sizeof (sz)); // blocking
      } catch (Gio::Error &ex) {
        LOG (debug) << "ae: " << ex.what ();
        return a;
      }

      if (read != sizeof(sz)) {
        LOG (debug) << "ae: reader: could not read size.";
        return a;
      }

      if (sz > AeProtocol::MAX_MESSAGE_SZ) {
        LOG (debug) << "ae: reader: message exceeds max size.";
        return a;
      }

      /* read message type */
      AeProtocol::MessageTypes mt;
      read = istream->read ((char*)&mt, sizeof (mt));
      if (read != sizeof (mt)) {
        LOG (debug) << "ae: reader: size of message type too short: " << sizeof(mt) << " != " << read;
        return a;
      }

      /* read message */
      gchar buffer[sz + 1]; buffer[sz] = '\0';
      bool s = istream->read_all (buffer, sz, read);

      if (!s) {
        LOG (debug) << "ae: reader: error while reading message (size: " << sz << ")";
        return a;
      }

      /* parse message */
      if (mt != AeProtocol::MessageTypes::Ack) {
        LOG (debug) << "ae: reader: did not get Ack message back!";
        return a;
      }

      LOG (debug) << "ae: send (sync) ACK received.";
      a.ParseFromString (buffer);
    }

    return a;
  }
}

