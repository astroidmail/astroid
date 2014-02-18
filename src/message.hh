# pragma once

# include <vector>

# include "proto.hh"
# include "astroid.hh"

using namespace std;

namespace Astroid {
  class Message : public Glib::Object {
    public:
      Message ();
      Message (ustring _fname);

      ustring fname;

      void load_message ();

  };

  class MessageThread : public Glib::Object {
    public:
      MessageThread ();
      MessageThread (ustring _tid);

      ustring threadid;
      vector<refptr<Message>> messages;

      void load_messages ();
      void reload_messages ();
  };
}

