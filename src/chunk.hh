# pragma once

# include <vector>
# include <map>
# include <atomic>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class Chunk : public Glib::Object {
    private:
      static atomic<uint> nextid;

    public:
      Chunk (GMimeObject *);
      ~Chunk ();

      int id;

      /* Chunk assumes ownership of these */
      GMimeObject *       mime_object;
      GMimeContentType *  content_type;
      ustring content_id;

      ustring get_content_type ();

      ustring viewable_text (bool);

      vector<refptr<Chunk>> kids;
      vector<refptr<Chunk>> siblings;
      refptr<Chunk> get_by_id (int, bool check_siblings = true);

      bool any_kids_viewable ();
      bool any_kids_viewable_and_preferred ();

      bool viewable   = false;
      bool attachment = false;
      bool preferred  = false;
      bool mime_message = false;
      bool isencrypted  = false;
      bool issigned     = false;
      bool signature  = false;

      refptr<Message> get_mime_message ();

      map<ustring, GMimeContentType *> viewable_types = {
        { "plain", g_mime_content_type_new ("text", "plain") },
        { "html" , g_mime_content_type_new ("text", "html") }
      };

      GMimeContentType * preferred_type = viewable_types["plain"];

      /* attachment specific stuff */
      ustring get_filename ();
      size_t  get_file_size ();
      refptr<Glib::ByteArray> contents ();

      bool save_to (string filename, bool overwrite = false);
      void open ();
      void save ();

    private:
      ustring _fname;
      void do_open (ustring);
  };
}

