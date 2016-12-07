# pragma once

# include <vector>

# include <notmuch.h>
# include <gmime/gmime.h>

# include "proto.hh"
# include "astroid.hh"
# include "utils/address.hh"

namespace Astroid {
  class Message : public Glib::Object {
    public:
      Message ();
      Message (ustring _fname);
      Message (ustring _mid, ustring _fname);
      Message (notmuch_message_t *, int _level);
      Message (GMimeMessage *);
      ~Message ();

      ustring fname;
      ustring mid;
      ustring tid;
      bool    in_notmuch;
      bool    has_file;
      bool    missing_content; // file does not have a gmimeobject nor a file, use
                               // notmuch cache. essentially means that the
                               // db is out of sync.
      ustring get_filename (ustring appendix = "");

      void load_message_from_file (ustring);
      void load_message (GMimeMessage *);
      void load_notmuch_cache ();
      void load_tags (Db *);
      void load_tags (notmuch_message_t *);

      void on_message_updated (Db *, ustring);
      void refresh (Db *);

      GMimeMessage * message;
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

      time_t  received_time;
      ustring date ();
      ustring pretty_date ();
      ustring pretty_verbose_date (bool = false);
      std::vector<ustring> tags;

      ustring viewable_text (bool, bool fallback_html = false);
      std::vector<refptr<Chunk>> attachments ();
      refptr<Chunk> get_chunk_by_id (int id);

      std::vector<refptr<Chunk>> mime_messages ();

      /* used by editmessage, returns the same as attachments () and mime_messages (),
       * but in the correct order. */
      std::vector<refptr<Chunk>> mime_messages_and_attachments ();

      refptr<Glib::ByteArray> contents ();
      refptr<Glib::ByteArray> raw_contents ();

      bool is_patch ();
      bool is_encrypted ();
      bool is_signed ();

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

      bool in_notmuch;
      refptr<NotmuchThread> thread;
      ustring subject;
      std::vector<refptr<Message>> messages;

      void load_messages (Db *);
      void add_message (ustring);
      void add_message (refptr<Chunk>);
      void reload_messages ();
  };
}

