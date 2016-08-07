# include <iostream>
# include <sys/time.h>
# include <sys/wait.h>

# include <boost/filesystem.hpp>
# include <gmime/gmime.h>
# include <glib.h>
# include <gio/gio.h>

# include "astroid.hh"
# include "db.hh"
# include "config.hh"
# include "compose_message.hh"
# include "message_thread.hh"
# include "account_manager.hh"
# include "log.hh"
# include "chunk.hh"
# include "crypto.hh"
# include "actions/action_manager.hh"
# include "actions/onmessage.hh"
# include "utils/address.hh"
# include "utils/ustring_utils.hh"
# include "plugin/manager.hh"

using namespace std;
namespace bfs = boost::filesystem;

namespace Astroid {
  ComposeMessage::ComposeMessage () {
    log << debug << "cm: initialize.." << endl;
    message = g_mime_message_new (true);

    d_message_sent.connect (
        sigc::mem_fun (this, &ComposeMessage::message_sent_event));
  }

  ComposeMessage::~ComposeMessage () {
    g_object_unref (message);

    //if (message_file != "") unlink(message_file.c_str());

    log << debug << "cm: deinitialized." << endl;
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
    g_mime_object_set_header (GMIME_OBJECT(message), "References",
        references.c_str());
  }

  void ComposeMessage::set_inreplyto (ustring _inreplyto) {
    inreplyto = _inreplyto;
    g_mime_object_set_header (GMIME_OBJECT(message), "In-Reply-To",
        inreplyto.c_str());
  }

  void ComposeMessage::build () {
    log << debug << "cm: build.." << endl;

    std::string body_content(body.str());

    /* attached signatures are handled in ::finalize */
    if (include_signature && !account->signature_attach) {
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

    g_mime_message_set_mime_part(message, GMIME_OBJECT(messagePart));

    g_object_unref(messagePart);
    g_object_unref(contentWrapper);
    g_object_unref(contentStream);
  }

  void ComposeMessage::load_message (ustring _mid, ustring fname) {
    set_id (_mid);
    Message msg (_mid, fname);

    Account * from = astroid->accounts->get_account_for_address (msg.sender);
    if (from == NULL) {
      log << warn << "cm: warning: unknown sending address, using default." << endl;
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
    // TODO: leaking astroid PID
    g_mime_message_set_message_id(message, id.c_str());

    /* inline signatures are handled in ::build */
    if (include_signature && account->signature_attach) {
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

      for (shared_ptr<Attachment> &a : attachments)
      {
        if (!a->valid) {
          log << error << "cm: invalid attachment: " << a->name << endl;
          // in practice this cannot happen since EditMessage will
          // not add an invalid attachment. In the case that it would
          // be added, there would be no way for the user to delete it
          // from the draft.
          continue;
        }

        if (a->is_mime_message) {

          GMimeMessagePart * mp = g_mime_message_part_new_with_message ("rfc822", (GMimeMessage*) a->message);
          g_mime_multipart_add (multipart, (GMimeObject *) mp);

          g_object_unref (mp);

        } else {

          GMimeStream * file_stream;

          if (a->on_disk) {

            file_stream = g_mime_stream_file_new (fopen(a->fname.c_str(), "r"));

          } else {

            file_stream = g_mime_stream_mem_new_with_byte_array (a->contents->gobj());
            g_mime_stream_mem_set_owner (GMIME_STREAM_MEM (file_stream), false);

          }

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

      } else {
        /* only sign */

        GMimeMultipartSigned * s_content = NULL;
        encryption_success = cy.sign (content, account->gpgkey, &s_content, &err);

        g_mime_message_set_mime_part (message, (GMimeObject *) s_content);
      }

      g_object_unref (content);

      if (!encryption_success) {
        encryption_error = err->message;
        log << error << "cm: failed encrypting or signing: " << encryption_error << endl;
      }
    }
  }

  void ComposeMessage::add_attachment (shared_ptr<Attachment> a) {
    attachments.push_back (a);
  }

  bool ComposeMessage::cancel_sending () {
    if (pid > 0) {
      log << warn << "cm: cancel sendmail pid: " << pid << endl;

      int r = kill (pid, SIGKILL);

      if (r == 0) {
        log << warn << "cm: sendmail killed." << endl;
      } else {
        log << error << "cm: could not kill sendmail." << endl;
      }
    }

    return true;
  }

  void ComposeMessage::send_threaded ()
  {
    log << info << "cm: sending (threaded).." << endl;
    Glib::Threads::Thread::create (
        [&] () {
          this->send (true);
        });
  }

  bool ComposeMessage::send (bool output) {
    dryrun = astroid->config().get<bool>("astroid.debug.dryrun_sending");

    /* Send the message */
    if (!dryrun) {
      if (output)
        log << warn << "cm: sending message from account: " << account->full_address () << endl;

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
          log << error << "cm: could not send message!" << endl;
        message_sent_result = false;
        d_message_sent ();
        pid = 0;
        return false;
      }

      /* connect channels */
      Glib::signal_io().connect (sigc::mem_fun (this, &ComposeMessage::log_out), stdout, Glib::IO_IN | Glib::IO_HUP);
      Glib::signal_io().connect (sigc::mem_fun (this, &ComposeMessage::log_err), stderr, Glib::IO_IN | Glib::IO_HUP);

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

      if (status == 0)
      {
        if (output)
          log << warn << "cm: message sent successfully!" << endl;

        if (account->save_sent) {
          using bfs::path;
          save_to = path(account->save_sent_to) / path(id + ":2,");
          if (output)
            log << info << "cm: saving message to: " << save_to << endl;

          write (save_to.c_str());
        }

        pid = 0;
        message_sent_result = true;
        d_message_sent ();
        return true;

      } else {
        if (output)
          log << error << "cm: could not send message!" << endl;
        message_sent_result = false;
        d_message_sent ();
        pid = 0;
        return false;
      }
    } else {
      ustring fname = "/tmp/" + id;
      if (output)
        log << warn << "cm: sending disabled in config, message written to: " << fname << endl;

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
      log << error << "cm: invalid fifo response" << endl;
    } else {
      Glib::ustring buf;

      ch_stdout->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      log << debug << "sendmail: " << buf << endl;

    }
    return true;
  }

  bool ComposeMessage::log_err (Glib::IOCondition cond) {
    if (cond == Glib::IO_HUP) {
      ch_stderr.clear();
      return false;
    }

    if ((cond & Glib::IO_IN) == 0) {
      log << error << "cm: invalid fifo response" << endl;
    } else {
      Glib::ustring buf;

      ch_stderr->read_line(buf);
      if (*(--buf.end()) == '\n') buf.erase (--buf.end());

      log << warn << "sendmail: " << buf << endl;
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
      log << info << "cm: sent message added to db." << endl;
    }

    emit_message_sent (message_sent_result);
  }

  ustring ComposeMessage::write_tmp () {
    char * temporaryFilePath;

    if (!bfs::is_directory ("/tmp")) {
      /* this fails if /tmp does not exist, typically in a chroot */
      log << warn << "cm: /tmp is not a directory, writing tmp files to current directory." << endl;
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

    log << info << "cm: wrote tmp file: " << message_file << endl;

    return message_file;
  }

  void ComposeMessage::write (ustring fname) {
    FILE * MessageFile = fopen(fname.c_str(), "w");

    GMimeStream * stream = g_mime_stream_file_new(MessageFile);
    g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);
    g_mime_stream_flush (stream);

    g_object_unref(stream);

    log << debug << "cm: wrote file: " << fname << endl;
  }

  ComposeMessage::Attachment::Attachment () {
    on_disk = false;
  }

  ComposeMessage::Attachment::Attachment (bfs::path p) {
    log << debug << "cm: at: construct from file." << endl;
    fname   = p;
    on_disk = true;

    std::string filename = fname.c_str ();

    GError * error = NULL;
    GFile * file = g_file_new_for_path(filename.c_str());
    GFileInfo * file_info = g_file_query_info(file,
        G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        G_FILE_QUERY_INFO_NONE, NULL, &error);

    if (error)
    {
      log << error << "cm: could not query file information." << endl;
      valid = false;
      g_object_unref (file);
      g_object_unref (file_info);
      return;
    }
    if (g_file_info_get_file_type(file_info) != G_FILE_TYPE_REGULAR)
    {
      log << error << "cm: attached file is not a regular file." << endl;
      valid = false;
      g_object_unref (file);
      g_object_unref (file_info);
      return;
    }

    content_type = g_file_info_get_content_type (file_info);
    name = fname.filename().c_str();
    valid = true;
    g_object_unref (file);
    g_object_unref (file_info);
  }

  ComposeMessage::Attachment::Attachment (refptr<Chunk> c) {
    log << debug << "cm: at: construct from chunk." << endl;
    name = c->get_filename ();
    on_disk = false;

    contents = c->contents ();

    const char * ct = g_mime_content_type_to_string (c->content_type);
    if (ct != NULL) {
      content_type = std::string (ct);
    } else {
      content_type = "application/octet-stream";
    }

    valid = true;
  }

  ComposeMessage::Attachment::Attachment (refptr<Message> msg) {
    log << debug << "cm: at: construct from message." << endl;
    name = msg->subject;
    on_disk = false;
    is_mime_message = true;

    content_type = "message/rfc822";
    message = (GMimeObject*) msg->message;
    g_object_ref (message);

    valid = true;
  }

  ComposeMessage::Attachment::~Attachment () {
    log << debug << "cm: at: deconstruct" << endl;
    if (message) g_object_unref (message);
  }

}

