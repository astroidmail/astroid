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
      Message (notmuch_message_t *);
      ~Message ();

      ustring fname;
      ustring mid;

      void load_message ();

      GMimeMessage * message;
      refptr<Chunk>     root;

      ustring sender;
      ustring subject;
      InternetAddressList * to ();
      InternetAddressList * cc ();
      InternetAddressList * bcc ();

      ustring date ();
      vector<ustring> tags ();

      ustring body ();
  };

  class MessageThread : public Glib::Object {
    public:
      MessageThread ();
      MessageThread (ustring _tid);

      ustring thread_id;
      ustring subject;
      vector<refptr<Message>> messages;

      void load_messages ();
      void reload_messages ();
  };
}

