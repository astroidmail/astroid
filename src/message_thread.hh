# pragma once

# include <vector>

# include <notmuch.h>
# include <gmime/gmime.h>

# include "proto.hh"
# include "astroid.hh"
# include "utils/address.hh"

namespace Astroid {
  class Message : public Glib::Object {
    friend MessageThread;

    public:
      Message ();
      Message (ustring _fname);
      Message (ustring _mid, ustring _fname);
      Message (notmuch_message_t *, int _level);
      Message (GMimeMessage *);
      Message (refptr<NotmuchMessage>);
      ~Message ();

      ustring fname;
      ustring mid;
      ustring tid;
      refptr<NotmuchMessage> nmmsg;
      bool    in_notmuch;
      bool    has_file;
      bool    missing_content; // file does not have a gmimeobject nor a file, use
                               // notmuch cache. essentially means that the
                               // db is out of sync.
      ustring get_filename (ustring appendix = "");

      void load_message_from_file (ustring);
      void load_message (GMimeMessage *);
      void load_notmuch_cache ();

      void on_message_updated (Db *, ustring);
      void refresh (Db *);

      GMimeMessage * message = NULL;
      refptr<Chunk>     root;
      int level = 0;

      ustring sender;
      ustring subject;
      InternetAddressList * to ();
      InternetAddressList * cc ();
      InternetAddressList * bcc ();
      InternetAddressList * other_to ();

      /* address list with all addresses in all headers beginning with to
       * and ending with from */
      AddressList all_to_from ();

      ustring references;
      ustring inreplyto;
      ustring reply_to;
      AddressList list_post ();

      time_t  time;
      ustring date ();
      ustring date_asctime ();
      ustring pretty_date ();
      ustring pretty_verbose_date (bool = false);
      std::vector<ustring> tags;

      ustring viewable_text (bool, bool fallback_html = false);
      std::vector<refptr<Chunk>> attachments ();
      refptr<Chunk> get_chunk_by_id (int id);

      std::vector<refptr<Chunk>> mime_messages ();

      /* used by editmessage, returns the same as attachments () and
       * mime_messages (), but in the correct order. */
      std::vector<refptr<Chunk>> mime_messages_and_attachments ();

      refptr<Glib::ByteArray> contents ();
      refptr<Glib::ByteArray> raw_contents ();

      bool is_patch ();
      bool is_different_subject ();
      bool is_encrypted ();
      bool is_signed ();
      bool is_list_post ();
      bool has_tag (ustring);

      void save ();
      void save_to (ustring);

      /* message changed signal */
      typedef enum {
        MESSAGE_TAGS_CHANGED,
      } MessageChangedEvent;

      typedef sigc::signal <void, Db *, Message *, MessageChangedEvent> type_signal_message_changed;
      type_signal_message_changed signal_message_changed ();

    protected:
      void emit_message_changed (Db *, MessageChangedEvent);
      type_signal_message_changed m_signal_message_changed;

      bool subject_is_different = true;
  };

  /* exceptions */
  class message_error : public std::runtime_error {
    public:
      message_error (const char *);

  };

  class MessageThread : public Glib::Object {
    public:
      MessageThread ();
      MessageThread (refptr<NotmuchThread>);
      ~MessageThread ();

      bool in_notmuch;
      ustring get_subject ();
      bool has_tag (ustring);

    private:
      ustring subject;
      ustring first_subject = "";
      void set_first_subject (ustring);
      bool first_subject_set = false;
      bool subject_is_different (ustring);

      void on_thread_updated (Db * db, ustring tid);
      void on_thread_changed (Db * db, ustring tid);

    public:
      refptr<NotmuchThread> thread;
      std::vector<refptr<Message>> messages;

      void load_messages (Db *);
      void add_message (ustring);
      void add_message (refptr<Chunk>);
  };
}

