# pragma once

# include <iostream>
# include <vector>
# include <string>
# include <functional>
# include <memory>
# include <boost/filesystem.hpp>
# include <thread>
# include <mutex>
# include <condition_variable>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

namespace bfs = boost::filesystem;

namespace Astroid {

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
      bool markdown = false;
      bool encrypt = false;
      bool sign    = false;

      bool markdown_success = false;
      ustring markdown_error = "";

      struct Attachment {
        public:
          Attachment ();
          Attachment (bfs::path);
          Attachment (refptr<Chunk>);
          Attachment (refptr<Message>);
          ~Attachment ();
          ustring name;
          bfs::path    fname;
          bool    is_mime_message = false;
          bool    dispostion_inline = false;
          bool    valid;

          refptr<Glib::ByteArray> contents;
          std::string             content_type;
          refptr<Message>         message;

          int     chunk_id = -1;
      };

      void add_attachment (std::shared_ptr<Attachment>);
      std::vector<std::shared_ptr<Attachment>> attachments;

      void load_message (ustring, ustring); // load draft or message as new

      void build ();    // call to build message from content
      void finalize (); // call before sending
      bool send ();
      void send_threaded ();
      bool cancel_sending ();
      ustring write_tmp (); // write message to tmpfile
      void write (ustring); // write message to some file
      void write (GMimeStream *); // write to stream

      /* encryption */
      bool encryption_success = false;
      ustring encryption_error = "";

    private:
      ustring message_file;
      bfs::path save_to;
      bool      dryrun;

      /* sendmail process */
      bool cancel_send_during_delay = false;
      int pid;

      std::thread send_thread;
      std::mutex  send_cancel_m;
      std::condition_variable  send_cancel_cv;

    public:
      /* message sent */
      typedef sigc::signal <void, bool> type_message_sent;
      type_message_sent message_sent ();

      void emit_message_sent (bool);

      bool message_sent_result;
      void message_sent_event ();
      Glib::Dispatcher d_message_sent;

      /* message send status update */
      typedef sigc::signal <void, bool, ustring> type_message_send_status;
      type_message_send_status message_send_status ();

      void emit_message_send_status (bool, ustring);

      bool    message_send_status_warn = false;
      ustring message_send_status_msg = "";

      void message_send_status_event ();
      Glib::Dispatcher d_message_send_status;

    protected:
      type_message_sent m_message_sent;
      type_message_send_status m_message_send_status;
  };
}
