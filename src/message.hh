# pragma once

# include <vector>

# include "proto.hh"
# include "gulp.hh"

using namespace std;

namespace Gulp {
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

