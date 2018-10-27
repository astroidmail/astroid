# pragma once

# include <giomm.h>
# include <mutex>

# include "messages.pb.h"

namespace Astroid {
  class AeProtocol {
    public:
      typedef enum _MessageTypes {
        /*
         * XXX: Add string value to map in MessageTypeStrings
         */

        Debug = 0,
        Ack,
        Info,
        Page,
        State,
        Indent,
        AllowRemoteImages,
        Focus,
        Navigate,
        Mark,
        Hidden,
        ClearMessages,
        AddMessage,
        UpdateMessage,
        RemoveMessage,
      } MessageTypes;

      static const char* MessageTypeStrings[];
      static const gsize MAX_MESSAGE_SZ = 200 * 1024 * 1024; // 200 MB

      static void send_message_async (
          MessageTypes mt,
          const ::google::protobuf::Message &m,
          Glib::RefPtr<Gio::OutputStream> ostream,
          std::mutex &);

      static AstroidMessages::Ack send_message_sync (
          MessageTypes mt,
          const ::google::protobuf::Message &m,
          Glib::RefPtr<Gio::OutputStream> ostream,
          std::mutex & m_ostream,
          Glib::RefPtr<Gio::InputStream>  istream,
          std::mutex & m_istream);

      static MessageTypes read_message (
          Glib::RefPtr<Gio::InputStream> istream,
          Glib::RefPtr<Gio::Cancellable> reader_cancel,
          std::vector<gchar> &buffer);

      /* exceptions */
      class ipc_error : public std::runtime_error {
        public:
          ipc_error (const char *);
      };

    private:
      static void send_message (
          MessageTypes mt,
          const ::google::protobuf::Message &m,
          Glib::RefPtr<Gio::OutputStream> ostream);
  };
}



