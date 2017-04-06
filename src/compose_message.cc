# include <iostream>
# include <sys/time.h>
# include <sys/wait.h>

# include <boost/filesystem.hpp>
# include <gmime/gmime.h>
# include <glib.h>
# include <gio/gio.h>
# include <thread>
# include <mutex>
# include <condition_variable>
# include <chrono>

# include "astroid.hh"
# include "db.hh"
# include "config.hh"
# include "compose_message.hh"
# include "message_thread.hh"
# include "account_manager.hh"
# include "chunk.hh"
# include "crypto.hh"
# include "actions/action_manager.hh"
# include "actions/onmessage.hh"
# include "utils/address.hh"
# include "utils/ustring_utils.hh"
# ifndef DISABLE_PLUGINS
  # include "plugin/manager.hh"
# endif

using namespace std;
namespace bfs = boost::filesystem;

namespace Astroid {
  ComposeMessage::ComposeMessage () {
    LOG (debug) << "cm: initialize..";
    message = g_mime_message_new (true);

    d_message_sent.connect (
        sigc::mem_fun (this, &ComposeMessage::message_sent_event));
    d_message_send_status.connect (
        sigc::mem_fun (this, &ComposeMessage::message_send_status_event));
  }

  ComposeMessage::~ComposeMessage () {
    if (send_thread.joinable ()) send_thread.join ();
    g_object_unref (message);

    //if (message_file != "") unlink(message_file.c_str());

    LOG (debug) << "cm: deinitialized.";
  }

  void ComposeMessage::set_from (Account *a) {
    // some of this stuff is more or less ripped from ner
    account = a;
    from = internet_address_mailbox_new (a->name.c_str(), a->email.c_str());
    g_mime_message_set_sender (message, internet_address_to_string (from, true));
  }

  void ComposeMessage::set_to (ustring _to) {
    to = _to;
    g_mime_object_set_header (GMIME_OBJECT(message), "To", to.c_str());
  }

  void ComposeMessage::set_cc (ustring _cc) {
    cc = _cc;
    g_mime_object_set_header (GMIME_OBJECT(message), "Cc", cc.c_str());
  }

  void ComposeMessage::set_bcc (ustring _bcc) {
    bcc = _bcc;
    g_mime_object_set_header (GMIME_OBJECT(message), "Bcc", bcc.c_str());
  }

  void ComposeMessage::set_subject (ustring _subject) {
    subject = _subject;
    g_mime_message_set_subject (message, subject.c_str());
  }

  void ComposeMessage::set_id (ustring _id) {
    id = _id;
  }

  void ComposeMessage::set_references (ustring _refs) {
    references = _refs;
    if (references.empty ()) {
      g_mime_header_list_remove (
          g_mime_object_get_header_list (GMIME_OBJECT(message)),
          "References");
    } else {
      g_mime_object_set_header (GMIME_OBJECT(message), "References",
          references.c_str());
    }
  }

  void ComposeMessage::set_inreplyto (ustring _inreplyto) {
    inreplyto = _inreplyto;
    if (inreplyto.empty ()) {
      g_mime_header_list_remove (
          g_mime_object_get_header_list (GMIME_OBJECT(message)),
          "In-Reply-To");
    } else {
      g_mime_object_set_header (GMIME_OBJECT(message), "In-Reply-To",
          inreplyto.c_str());
    }
  }

  void ComposeMessage::build () {
    LOG (debug) << "cm: build..";

    std::string body_content(body.str());

    /* attached signatures are handled in ::finalize */
    if (include_signature && account && !account->signature_attach) {
      LOG (debug) << "cm: adding inline signature..";
      std::ifstream s (account->signature_file.c_str ());
      std::ostringstream sf;
      sf << s.rdbuf ();
      s.close ();
      body_content += sf.str ();
    }

    GMimeStream * contentStream = g_mime_stream_mem_new_with_buffer(body_content.c_str(), body_content.size());
    GMimePart * messagePart = g_mime_part_new_with_type ("text", "plain");

    g_mime_object_set_content_type_parameter ((GMimeObject *) messagePart, "charset", astroid->config().get<string>("editor.charset").c_str());
    g_mime_object_set_content_type_parameter ((GMimeObject *) messagePart, "format", "flowed");

    GMimeDataWrapper * contentWrapper = g_mime_data_wrapper_new_with_stream(contentStream, GMIME_CONTENT_ENCODING_DEFAULT);

    g_mime_part_set_content_encoding (messagePart, GMIME_CONTENT_ENCODING_QUOTEDPRINTABLE);
    g_mime_part_set_content_object (messagePart, contentWrapper);

    GMimeObject * part = g_mime_message_get_mime_part (message);
    g_mime_message_set_mime_part(message, GMIME_OBJECT(messagePart));
    if (part) g_object_unref (part);

    g_object_unref(messagePart);
    g_object_unref(contentWrapper);
    g_object_unref(contentStream);
  }

  void ComposeMessage::load_message (ustring _mid, ustring fname) {
    set_id (_mid);
    Message msg (_mid, fname);

    Account * from = astroid->accounts->get_account_for_address (msg.sender);
    if (from == NULL) {
      LOG (warn) << "cm: warning: unknown sending address, using default.";
      from = &(astroid->accounts->accounts[astroid->accounts->default_account]);
    }
    set_from (from);

    char * cto = internet_address_list_to_string (msg.to(), false);
    if (cto) set_to (cto);

    cto = internet_address_list_to_string (msg.cc(), false);
    if (cto) set_cc (cto);

    cto = internet_address_list_to_string (msg.bcc(), false);
    if (cto) set_bcc (cto);

    set_references (msg.references);
    set_inreplyto (msg.inreplyto);

    set_subject (msg.subject);

    body << msg.viewable_text (false);

  }

  void ComposeMessage::finalize () {
    /* make message ready to be sent */
    LOG (debug) << "cm: finalize..";

    /* again: ripped more or less from ner */

    /*
    FILE * file = fopen(_messageFile.c_str(), "r");
    GMimeStream * stream = g_mime_stream_file_new(file);
    GMimeParser * parser = g_mime_parser_new_with_stream(stream);
    GMimeMessage * message = g_mime_parser_construct_message(parser);
    */

    /* set user agent */
    ustring ua = "";

# ifndef DISABLE_PLUGINS
    if (!astroid->plugin_manager->astroid_extension->get_user_agent (ua)) {
# endif

      ua = astroid->config ().get<string> ("mail.user_agent");
      UstringUtils::trim (ua);

      if (ua == "default") ua = astroid->user_agent;
# ifndef DISABLE_PLUGINS
    }
# endif

    if (!ua.empty ()) {
      g_mime_object_set_header (GMIME_OBJECT(message), "User-Agent", ua.c_str());
    }


    /* add date to the message */
    struct timeval timeValue;
    struct timezone timeZone;

    gettimeofday(&timeValue, &timeZone);

    g_mime_message_set_date(message, timeValue.tv_sec, -100 * timeZone.tz_minuteswest / 60);

    /* Give the message an ID */
    g_mime_message_set_message_id(message, id.c_str());

    /* inline signatures are handled in ::build */
    if (include_signature && account && account->signature_attach) {
      shared_ptr<ComposeMessage::Attachment> sa (
        new ComposeMessage::Attachment (account->signature_file));

      /* this could be an .vcf, so lets not try to be too smart here */
      add_attachment (sa);
    }

    /* attachments (most of this copied from ner) */
    if (attachments.size() > 0)
    {
      GMimeMultipart * multipart = g_mime_multipart_new_with_subtype("mixed");
      g_mime_multipart_add (multipart, (GMimeObject*) message->mime_part);
      g_mime_message_set_mime_part (message, (GMimeObject*) multipart);

      /* not unreffing message->mime_part here since it is reused */

      for (shared_ptr<Attachment> &a : attachments)
      {
        if (!a->valid) {
          LOG (error) << "cm: invalid attachment: " << a->name;
          // in practice this cannot happen since EditMessage will
          // not add an invalid attachment. In the case that it would
          // be added, there would be no way for the user to delete it
          // from the draft.
          continue;
        }

        if (a->is_mime_message) {

          GMimeMessagePart * mp = g_mime_message_part_new_with_message ("rfc822", (GMimeMessage*) a->message->message);
          g_mime_multipart_add (multipart, (GMimeObject *) mp);

          g_object_unref (mp);

        } else {

          GMimeStream * file_stream;


          file_stream = g_mime_stream_mem_new_with_byte_array (a->contents->gobj());
          g_mime_stream_mem_set_owner (GMIME_STREAM_MEM (file_stream), false);


          GMimeDataWrapper * data = g_mime_data_wrapper_new_with_stream (file_stream,
              GMIME_CONTENT_ENCODING_DEFAULT);

          GMimeContentType * contentType = g_mime_content_type_new_from_string (a->content_type.c_str());

          GMimePart * part =
            g_mime_part_new_with_type(g_mime_content_type_get_media_type (contentType),
            g_mime_content_type_get_media_subtype (contentType));
          g_mime_part_set_content_object (part, data);
          g_mime_part_set_filename (part, a->name.c_str());

          if (a->dispostion_inline) {
            g_mime_object_set_disposition (GMIME_OBJECT(part), "inline");
          } else {
            g_mime_part_set_content_encoding (part, GMIME_CONTENT_ENCODING_BASE64);
          }


          g_mime_multipart_add (multipart, (GMimeObject*) part);

          g_object_unref (part);
          g_object_unref (contentType);
          g_object_unref (file_stream);
          g_object_unref (data);
        }

      }

      g_object_unref(multipart);
    }

    /* encryption */
    encryption_success = false;
    encryption_error = "";
    GError * err = NULL;

    if (encrypt || sign) {
      GMimeObject * content = g_mime_message_get_mime_part (message);

      Crypto cy ("application/pgp-encrypted");

      if (encrypt) {

        GMimeMultipartEncrypted * e_content = NULL;
        encryption_success = cy.encrypt (content, sign, account->gpgkey, from, AddressList (to) + AddressList (cc) + AddressList (bcc), &e_content, &err);

        g_mime_message_set_mime_part (message, (GMimeObject *) e_content);
        g_object_unref (e_content);

      } else {
        /* only sign */

        GMimeMultipartSigned * s_content = NULL;
        encryption_success = cy.sign (content, account->gpgkey, &s_content, &err);

        g_mime_message_set_mime_part (message, (GMimeObject *) s_content);
        g_object_unref (s_content);
      }

      g_object_unref (content);

      if (!encryption_success) {
        encryption_error = err->message;
        LOG (error) << "cm: failed encrypting or signing: " << encryption_error;
      }
    }
  }

  void ComposeMessage::add_attachment (shared_ptr<Attachment> a) {
    attachments.push_back (a);
  }

  bool ComposeMessage::cancel_sending () {
    LOG (warn) << "cm: cancel sendmail pid: " << pid;
    std::lock_guard<std::mutex> lk (send_cancel_m);

    cancel_send_during_delay = true;

    if (pid > 0) {

      int r = kill (pid, SIGKILL);

      if (r == 0) {
        LOG (warn) << "cm: sendmail killed.";
      } else {
        LOG (error) << "cm: could not kill sendmail.";
      }
    }

    send_cancel_cv.notify_one ();
    if (send_thread.joinable ()) {
      send_thread.detach ();
    }

    return true;
  }

  void ComposeMessage::send_threaded ()
  {
    LOG (info) << "cm: sending (threaded)..";
    cancel_send_during_delay = false;
    send_thread = std::thread (&ComposeMessage::send, this, true);
  }

  bool ComposeMessage::send (bool output) {

    dryrun = astroid->config().get<bool>("astroid.debug.dryrun_sending");

    message_send_status_warn = false;
    message_send_status_msg  = "";

    unsigned int delay = astroid->config ().get<unsigned int> ("mail.send_delay");
    std::unique_lock<std::mutex> lk (send_cancel_m);

    while (delay > 0 && !cancel_send_during_delay) {
      LOG (debug) << "cm: sending in " << delay << " seconds..";
      if (astroid->hint_level () < 1) {
        /* TODO: replace C-c with the actual keybinding configured by the user */
        message_send_status_msg = ustring::compose ("sending message in %1 seconds... Press C-c to cancel!", delay);
      } else {
        message_send_status_msg = ustring::compose ("sending message in %1 seconds...", delay);
      }
      d_message_send_status ();
      std::chrono::seconds sec (1);
      send_cancel_cv.wait_until (lk, std::chrono::system_clock::now () + sec, [&] { return cancel_send_during_delay; });
      delay--;
    }

    if (cancel_send_during_delay) {
      LOG (error) << "cm: cancelled sending before message could be sent.";
      message_send_status_msg = "sending message... cancelled before sending.";
      message_send_status_warn = true;
      d_message_send_status ();

      message_sent_result = false;
      d_message_sent ();
      pid = 0;
      return false;
    }

    lk.unlock ();

    if (astroid->hint_level () < 1) {
      /* TODO: replace C-c with the actual keybinding configured by the user */
      message_send_status_msg = "sending message... Press C-c to cancel!";
    } else {
      message_send_status_msg = "sending message...";
    }
    d_message_send_status ();

    /* Send the message */
    if (!dryrun) {
      if (output)
        LOG (warn) << "cm: sending message from account: " << account->full_address ();

      ustring send_command = account->sendmail;
      vector<string> args = Glib::shell_parse_argv (send_command);
      try {
        Glib::spawn_async_with_pipes ("",
                          args,
                          Glib::SPAWN_DO_NOT_REAP_CHILD |
                          Glib::SPAWN_SEARCH_PATH,
                          sigc::slot <void> (),
                          &pid,
                          &stdin,
                          &stdout,
                          &stderr
                          );
      } catch (Glib::SpawnError &ex) {
        if (output)
          LOG (error) << "cm: could not send message!";

        message_send_status_msg = "message could not be sent!";
        message_send_status_warn = true;
        d_message_send_status ();

        message_sent_result = false;
        d_message_sent ();
        pid = 0;
        return false;
      }

      /* connect channels */
      c_ch_stdout = Glib::signal_io().connect (sigc::mem_fun (this, &ComposeMessage::log_out), stdout, Glib::IO_IN | Glib::IO_HUP);
      c_ch_stderr = Glib::signal_io().connect (sigc::mem_fun (this, &ComposeMessage::log_err), stderr, Glib::IO_IN | Glib::IO_HUP);


      ch_stdout = Glib::IOChannel::create_from_fd (stdout);
      ch_stderr = Glib::IOChannel::create_from_fd (stderr);

      FILE * sendMailPipe = fdopen (stdin, "w");

      /* write message to sendmail */
      GMimeStream * sendMailStream = g_mime_stream_file_new(sendMailPipe);
      g_mime_stream_file_set_owner(GMIME_STREAM_FILE(sendMailStream), false);
      g_mime_object_write_to_stream(GMIME_OBJECT(message), sendMailStream);
      g_mime_stream_flush (sendMailStream);

      g_object_unref(sendMailStream);
      fclose (sendMailPipe);

      /* wait for sendmail to finish */
      int status;
      waitpid (pid, &status, 0);
      g_spawn_close_pid (pid);

      c_ch_stderr.disconnect();
      c_ch_stdout.disconnect();

      ::close (stdin);
      ::close (stdout);
      ::close (stderr);

      if (status == 0)
      {
        if (output)
          LOG (warn) << "cm: message sent successfully!";

        if (account->save_sent) {
          using bfs::path;
          save_to = path(account->save_sent_to) / path(id + ":2,");
          if (output)
            LOG (info) << "cm: saving message to: " << save_to;

          write (save_to.c_str());
        }

        message_send_status_msg = "message sent successfully!";
        message_send_status_warn = false;
        d_message_send_status ();

        pid = 0;
        message_sent_result = true;
        d_message_sent ();
        return true;

      } else {
        if (output)
          LOG (error) << "cm: could not send message!";

        message_send_status_msg = "message could not be sent!";
        message_send_status_warn = true;
        d_message_send_status ();

        message_sent_result = false;
        d_message_sent ();
        pid = 0;
        return false;
      }
    } else {
      ustring fname = "/tmp/" + id;
      if (output)
        LOG (warn) << "cm: sending disabled in config, message written to: " << fname;
      message_send_status_msg = "sending disabled, message written to: " + fname;
      message_send_status_warn = true;
      d_message_send_status ();

      write (fname);
      message_sent_result = false;
      d_message_sent ();
      pid = 0;
      return false;
    }
  }

  bool ComposeMessage::log_out (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stdout.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      LOG (error) << "cm: invalid fifo response";
    } else {
      Glib::ustring buf;

      ch_stdout->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      LOG (debug) << "sendmail: " << buf;

    }
    return true;
  }

  bool ComposeMessage::log_err (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stderr.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      LOG (error) << "cm: invalid fifo response";
    } else {
      Glib::ustring buf;

      ch_stderr->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      LOG (warn) << "sendmail: " << buf;
    }
    return true;
  }

  /* signals */
  ComposeMessage::type_message_sent
    ComposeMessage::message_sent ()
  {
    return m_message_sent;
  }

  void ComposeMessage::emit_message_sent (bool res) {
    m_message_sent.emit (res);
  }

  void ComposeMessage::message_sent_event () {
    /* add to notmuch with sent tag (on main GUI thread) */
    if (!dryrun && message_sent_result && account->save_sent) {
      astroid->actions->doit (refptr<Action> (
            new AddSentMessage (save_to.c_str (), account->additional_sent_tags)));
      LOG (info) << "cm: sent message added to db.";
    }

    emit_message_sent (message_sent_result);
  }

  ComposeMessage::type_message_send_status
    ComposeMessage::message_send_status ()
  {
    return m_message_send_status;
  }

  void ComposeMessage::emit_message_send_status (bool warn, ustring msg) {
    m_message_send_status.emit (warn, msg);
  }

  void ComposeMessage::message_send_status_event () {
    emit_message_send_status (message_send_status_warn, message_send_status_msg);
  }

  ustring ComposeMessage::write_tmp () {
    char * temporaryFilePath;

    if (!bfs::is_directory ("/tmp")) {
      /* this fails if /tmp does not exist, typically in a chroot */
      LOG (warn) << "cm: /tmp is not a directory, writing tmp files to current directory.";
      temporaryFilePath = strdup("tmp-astroid-compose-XXXXXX");
    } else {
      temporaryFilePath = strdup("/tmp/astroid-compose-XXXXXX");
    }

    int fd = mkstemp(temporaryFilePath);
    message_file = temporaryFilePath;
    free(temporaryFilePath);

    GMimeStream * stream = g_mime_stream_fs_new(fd);
    g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);
    g_mime_stream_flush (stream);

    g_object_unref(stream);

    LOG (info) << "cm: wrote tmp file: " << message_file;

    return message_file;
  }

  void ComposeMessage::write (ustring fname) {
    if (bfs::exists (fname.c_str ())) unlink (fname.c_str ());

    FILE * MessageFile = fopen(fname.c_str(), "w");

    GMimeStream * stream = g_mime_stream_file_new(MessageFile);
    g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);
    g_mime_stream_flush (stream);

    g_object_unref(stream);

    LOG (debug) << "cm: wrote file: " << fname;
  }

  ComposeMessage::Attachment::Attachment () {
  }

  ComposeMessage::Attachment::Attachment (bfs::path p) {
    LOG (debug) << "cm: at: construct from file.";
    fname   = p;

    std::string filename = fname.c_str ();

    GError * error = NULL;
    GFile * file = g_file_new_for_path(filename.c_str());
    GFileInfo * file_info = g_file_query_info(file,
        G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (error)
    {
      LOG (error) << "cm: could not query file information.";
      valid = false;
      g_object_unref (file);
      g_object_unref (file_info);
      return;
    }

    if (g_file_info_get_file_type(file_info) != G_FILE_TYPE_REGULAR)
    {
      LOG (error) << "cm: attached file is not a regular file.";
      valid = false;
      g_object_unref (file);
      g_object_unref (file_info);
      return;
    }

    content_type = g_file_info_get_content_type (file_info);
    name = fname.filename().c_str();
    valid = true;

    if (content_type == "message/rfc822") {
      LOG (debug) << "cm: attachment is mime message.";
      message = refptr<Message> (new Message(fname.c_str ()));

      is_mime_message = true;

      name = message->subject;

    } else {
      /* load into byte array */
      refptr<Gio::File> fle = Glib::wrap (file, false);
      refptr<Gio::FileInputStream> istr = fle->read ();

      refptr<Glib::Bytes> b;
      contents = Glib::ByteArray::create ();

      do {
        b = istr->read_bytes (4096, refptr<Gio::Cancellable>(NULL));

        if (b) {
          gsize s = b->get_size ();
          if (s <= 0) break;
          contents->append ((const guint8 *) b->get_data (s), s);
        }

      } while (b);
    }

    g_object_unref (file);
    g_object_unref (file_info);
  }

  ComposeMessage::Attachment::Attachment (refptr<Chunk> c) {
    LOG (debug) << "cm: at: construct from chunk.";
    name = c->get_filename ();
    valid = true;

    /* used by edit message when deleting attachment */
    chunk_id = c->id;

    if (c->mime_message) {
      content_type = "message/rfc822";
      is_mime_message = true;

      message = refptr<Message> (new Message(GMIME_MESSAGE(c->mime_object)));

      g_object_ref (c->mime_object); // should be cleaned by Message : Glib::Object

      name = message->subject;

    } else {

      contents = c->contents ();

      const char * ct = g_mime_content_type_to_string (c->content_type);
      if (ct != NULL) {
        content_type = std::string (ct);
      } else {
        content_type = "application/octet-stream";
      }
    }
  }

  ComposeMessage::Attachment::Attachment (refptr<Message> msg) {
    LOG (debug) << "cm: at: construct from message.";
    name = msg->subject;
    is_mime_message = true;

    content_type = "message/rfc822";
    message = msg;

    valid = true;
  }

  ComposeMessage::Attachment::~Attachment () {
    LOG (debug) << "cm: at: deconstruct";
  }

}

