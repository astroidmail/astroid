# pragma once

# include <vector>

# include <notmuch.h>
# include <gmime/gmime.h>

# include "proto.hh"
# include "astroid.hh"

using namespace std;

namespace Astroid {
  class Message : public Glib::Object {
    public:
      Message ();
      Message (ustring _fname);
      Message (ustring _mid, ustring _fname);
      Message (notmuch_message_t *);
      ~Message ();

      ustring fname;
      ustring mid;
      bool    in_notmuch;

      void load_message ();
      void load_tags (Db *);
      void load_tags (notmuch_message_t *);

      GMimeMessage * message;
      refptr<Chunk>     root;

      ustring sender;
      ustring subject;
      InternetAddressList * to ();
      InternetAddressList * cc ();
      InternetAddressList * bcc ();

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
      void reload_messages ();
  };
}

