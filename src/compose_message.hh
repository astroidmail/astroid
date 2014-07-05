# pragma once

# include <iostream>
# include <boost/filesystem.hpp>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

using namespace std;
using namespace boost::filesystem;

namespace Astroid {
  class ComposeMessage {
    public:
      ComposeMessage  ();
      ~ComposeMessage ();

      GMimeMessage    * message;
      Account         * account;
      InternetAddress * from;

      ustring to, cc, bcc, id, subject;

      ostringstream body;

      void set_from (Account *);
      void set_to   (ustring);
      void set_cc   (ustring);
      void set_bcc  (ustring);
      void set_subject (ustring);
      void set_id   (ustring);

      void add_attachment (path);

      void load_message (ustring, ustring); // load draft or message as new

      void build ();    // call to build message from content
      void finalize (); // call before sending
      bool send ();
      ustring write_tmp (); // write message to tmpfile
      void write (ustring); // write message to some file

    private:
      ustring message_file;
  };
}
