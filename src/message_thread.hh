# pragma once

# include <vector>

# include <notmuch.h>
# include <gmime/gmime.h>

# include "proto.hh"
# include "astroid.hh"
# include "utils/address.hh"

using namespace std;

namespace Astroid {
  class Message : public Glib::Object {
    public:
      Message ();
      Message (ustring _fname);
      Message (ustring _mid, ustring _fname);
      Message (notmuch_message_t *);
      Message (GMimeMessage *);
      ~Message ();

      ustring fname;
      ustring mid;
      bool    in_notmuch;
      bool    has_file;

      void load_message_from_file (ustring);
      void load_message (GMimeMessage *);
      void load_tags (Db *);
      void load_tags (notmuch_message_t *);

      GMimeMessage * message;
      refptr<Chunk>     root;

      ustring sender;
      ustring subject;
      InternetAddressList * to ();
      InternetAddressList * cc ();
      InternetAddressList * bcc ();

      /* address list with all addresses in all headers beginning with to
       * and ending with from */
      AddressList all_to_from ();

      ustring references;
      ustring inreplyto;
      ustring reply_to;

      ustring date ();
      ustring pretty_date ();
      ustring pretty_verbose_date ();
      vector<ustring> tags;

      ustring viewable_text (bool);
      vector<refptr<Chunk>> attachments ();
      refptr<Chunk> get_chunk_by_id (int id);

      vector<refptr<Chunk>> mime_messages ();

      bool is_patch ();

      void save ();
      void save_to (ustring);
  };

  /* exceptions */
  class message_error : public runtime_error {
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
      vector<refptr<Message>> messages;

      void load_messages (Db *);
      void add_message (ustring);
      void add_message (refptr<Chunk>);
      void reload_messages ();
  };
}

