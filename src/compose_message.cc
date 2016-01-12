# include <iostream>
# include <sys/time.h>

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

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  ComposeMessage::ComposeMessage () {
    log << debug << "cm: initialize.." << endl;
    message = g_mime_message_new (true);

    d_message_sent.connect (
        sigc::mem_fun (this, &ComposeMessage::dispatch_emit_message_sent));
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

    GMimeStream * contentStream = g_mime_stream_mem_new_with_buffer(body_content.c_str(), body_content.size());
    GMimePart * messagePart = g_mime_part_new_with_type ("text", "plain");

    g_mime_object_set_content_type_parameter ((GMimeObject *) messagePart, "charset", astroid->config->config.get<string>("editor.charset").c_str());
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
    g_mime_object_set_header (GMIME_OBJECT(message), "User-Agent", astroid->user_agent.c_str());


    /* add date to the message */
    struct timeval timeValue;
    struct timezone timeZone;

    gettimeofday(&timeValue, &timeZone);

    g_mime_message_set_date(message, timeValue.tv_sec, -100 * timeZone.tz_minuteswest / 60);

    /* Give the message an ID */
    // TODO: leaking astroid PID
    g_mime_message_set_message_id(message, id.c_str());

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
        g_mime_part_set_content_encoding (part, GMIME_CONTENT_ENCODING_BASE64);
        g_mime_part_set_filename (part, a->name.c_str());

        g_mime_multipart_add (multipart, (GMimeObject*) part);
        g_object_unref (part);
        g_object_unref (contentType);
        g_object_unref (file_stream);
        g_object_unref (data);
      }

      g_object_unref(multipart);
    }
  }

  void ComposeMessage::add_attachment (shared_ptr<Attachment> a) {
    attachments.push_back (a);
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
    bool dryrun = astroid->config->config.get<bool>("astroid.debug.dryrun_sending");

    /* Send the message */
    if (!dryrun) {
      if (output)
        log << warn << "cm: sending message from account: " << account->full_address () << endl;
      ustring send_command = account->sendmail ;
      FILE * sendMailPipe = popen(send_command.c_str(), "w");
      GMimeStream * sendMailStream = g_mime_stream_file_new(sendMailPipe);
      g_mime_stream_file_set_owner(GMIME_STREAM_FILE(sendMailStream), false);
      g_mime_object_write_to_stream(GMIME_OBJECT(message), sendMailStream);
      g_object_unref(sendMailStream);

      int status = pclose(sendMailPipe);

      if (status == 0)
      {
        if (output)
          log << warn << "cm: message sent successfully!" << endl;

        if (account->save_sent) {
          path save_to = path(account->save_sent_to) / path(id + ":2,");
          if (output)
            log << info << "cm: saving message to: " << save_to << endl;

          write (save_to.c_str());

          /* add to notmuch with sent tag */
          Db db (Db::DbMode::DATABASE_READ_WRITE);
          lock_guard<Db> lk (db);
          db.add_sent_message (save_to.c_str(), account->additional_sent_tags);
          if (output)
            log << info << "cm: sent message added to db." << endl;
        }

        message_sent_result = true;
        d_message_sent ();
        return true;

      } else {
        if (output)
          log << error << "cm: could not send message!" << endl;
        message_sent_result = false;
        d_message_sent ();
        return false;
      }
    } else {
      ustring fname = "/tmp/" + id;
      if (output)
        log << warn << "cm: sending disabled in config, message written to: " << fname << endl;

      write (fname);
      message_sent_result = false;
      d_message_sent ();
      return false;
    }
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

  void ComposeMessage::dispatch_emit_message_sent () {
    emit_message_sent (message_sent_result);
  }

  ustring ComposeMessage::write_tmp () {
    char * temporaryFilePath = strdup("/tmp/astroid-compose-XXXXXX");
    int fd = mkstemp(temporaryFilePath);
    message_file = temporaryFilePath;
    free(temporaryFilePath);

    GMimeStream * stream = g_mime_stream_fs_new(fd);
    g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);

    g_object_unref(stream);

    log << info << "cm: wrote tmp file: " << message_file << endl;

    return message_file;
  }

  void ComposeMessage::write (ustring fname) {
    FILE * MessageFile = fopen(fname.c_str(), "w");
    GMimeStream * stream = g_mime_stream_file_new(MessageFile);
    g_mime_object_write_to_stream(GMIME_OBJECT(message), stream);
    g_object_unref(stream);
    log << debug << "cm: wrote file: " << fname << endl;
  }

  ComposeMessage::Attachment::Attachment (path p) {
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
      content_type = string (ct);
    } else {
      content_type = "application/octet-stream";
    }

    valid = true;
  }

  ComposeMessage::Attachment::~Attachment () {
    log << debug << "cm: at: deconstruct" << endl;
  }

}

