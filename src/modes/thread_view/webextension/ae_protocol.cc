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
    gsize written = 0;
    bool  s = false;

    m.SerializeToString (&o);

    /* send size of message */
    gsize sz = o.size ();
    s = ostream->write_all ((char*) &sz, sizeof(sz), written);

    /* send message type */
    s &= ostream->write_all ((char*) &mt, sizeof (mt), written);

    /* send message */
    s &= ostream->write_all (o, written);
    ostream->flush ();

    if (!s) {
      LOG (error) << "ae: could not write message!";
      throw ipc_error ("could not write message.");
    } else {
      LOG (debug) << "ae: wrote: " << written << " of " << o.size () << " bytes.";
    }
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
      std::string msg_str;

      auto mt = read_message (
          istream,
          Glib::RefPtr<Gio::Cancellable> (NULL),
          msg_str);

      /* parse message */
      if (mt != AeProtocol::MessageTypes::Ack) {
        LOG (debug) << "ae: reader: did not get Ack message back!";
        return a;
      }

      LOG (debug) << "ae: send (sync) ACK received.";
      a.ParseFromString (msg_str);
    }

    return a;
  }

  AeProtocol::MessageTypes AeProtocol::read_message (
      Glib::RefPtr<Gio::InputStream> istream,
      Glib::RefPtr<Gio::Cancellable> reader_cancel,
      std::string & msg_str)
  {
    gsize read = 0;
    bool  s    = false;

    /* read message size */
    gsize msg_sz = 0;
    s = istream->read_all ((char *) &msg_sz, sizeof (msg_sz), read, reader_cancel);

    if (!s || read != sizeof (msg_sz)) {
      throw ipc_error ("could not read message size");
    }

    if (msg_sz > AeProtocol::MAX_MESSAGE_SZ) {
      throw ipc_error ("message exceeds maximum size.");
    }

    AeProtocol::MessageTypes mt;
    s = istream->read_all ((char*) &mt, sizeof (mt), read, reader_cancel);

    if (!s || read != sizeof (mt)) {
      throw ipc_error ("could not read message type");
    }

    /* read message */
    gchar buffer[msg_sz + 1]; buffer[msg_sz] = '\0';
    s = istream->read_all (buffer, msg_sz, read, reader_cancel);

    if (!s || read != msg_sz) {
      LOG (error) << "reader: error while reading message (size: " << msg_sz << ")";
      throw ipc_error ("could not read message");
    }

    msg_str = std::string (buffer, msg_sz);
    return mt;
  }


  /***************
   * Exceptions
   ***************/

  AeProtocol::ipc_error::ipc_error (const char * w) : runtime_error (w)
  {
  }
}

