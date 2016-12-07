# pragma once

# include <iostream>
# include <vector>
# include <string>
# include <functional>
# include <memory>
# include <boost/filesystem.hpp>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

namespace bfs = boost::filesystem;

namespace Astroid {

  struct GLibDeleter
  {
    constexpr GLibDeleter() = default;

    void operator()(void * object)
    {
      g_object_unref(object);
    }
  };

  typedef std::unique_ptr<void, GLibDeleter> GLibPointer;

  class ComposeMessage : public sigc::trackable {
    public:
      ComposeMessage  ();
      ~ComposeMessage ();

      GMimeMessage    * message = NULL;
      Account         * account = NULL;
      InternetAddress * from    = NULL;

      ustring to, cc, bcc, id, subject, references, inreplyto;

      std::ostringstream body;

      void set_from (Account *);
      void set_to   (ustring);
      void set_cc   (ustring);
      void set_bcc  (ustring);
      void set_subject (ustring);
      void set_id   (ustring);
      void set_inreplyto (ustring);
      void set_references (ustring);

      bool include_signature = false;
      bool encrypt = false;
      bool sign    = false;

      struct Attachment {
        public:
          Attachment ();
          Attachment (bfs::path);
          Attachment (refptr<Chunk>);
          Attachment (refptr<Message>);
          ~Attachment ();
          ustring name;
          bfs::path    fname;
          bool    on_disk;
          bool    is_mime_message = false;
          bool    dispostion_inline = false;
          bool    valid;

          refptr<Glib::ByteArray> contents;
          std::string             content_type;
          GMimeObject *           message = NULL;

          int     chunk_id = -1;
      };

      void add_attachment (std::shared_ptr<Attachment>);
      std::vector<std::shared_ptr<Attachment>> attachments;

      void load_message (ustring, ustring); // load draft or message as new

      void build ();    // call to build message from content
      void finalize (); // call before sending
      bool send (bool = true);
      void send_threaded ();
      bool cancel_sending ();
      ustring write_tmp (); // write message to tmpfile
      void write (ustring); // write message to some file

      /* encryption */
      bool encryption_success = false;
      ustring encryption_error = "";

    private:
      ustring message_file;
      bfs::path save_to;
      bool      dryrun;

      /* sendmail process */
      int pid;
      int stdin;
      int stdout;
      int stderr;
      refptr<Glib::IOChannel> ch_stdout;
      refptr<Glib::IOChannel> ch_stderr;

      bool log_out (Glib::IOCondition);
      bool log_err (Glib::IOCondition);

    public:
      /* message sent */
      typedef sigc::signal <void, bool> type_message_sent;
      type_message_sent message_sent ();

      void emit_message_sent (bool);

      bool message_sent_result;
      void message_sent_event ();
      Glib::Dispatcher d_message_sent;

    protected:
      type_message_sent m_message_sent;
  };
}
