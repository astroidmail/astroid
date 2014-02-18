# pragma once

# include <vector>

# include <notmuch.h>

# include "proto.hh"
# include "astroid.hh"

using namespace std;

namespace Astroid {
  class Message : public Glib::Object {
    public:
      Message ();
      Message (ustring _fname);
      Message (notmuch_message_t *);

      ustring fname;

      ustring mid;

      void load_message ();

  };

  class MessageThread : public Glib::Object {
    public:
      MessageThread ();
      MessageThread (ustring _tid);

      ustring thread_id;
      vector<refptr<Message>> messages;

      void load_messages ();
      void reload_messages ();
  };
}

