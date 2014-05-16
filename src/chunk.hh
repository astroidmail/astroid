# pragma once

# include <vector>
# include <map>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class Chunk : public Glib::Object {
    public:
      Chunk (GMimeObject *);
      ~Chunk ();

      /* Chunk assumes ownership of these */
      GMimeObject *       mime_object;
      GMimeContentType *  content_type;
      ustring content_id;

      ustring body (bool);

      vector<refptr<Chunk>> kids;
      vector<refptr<Chunk>> siblings;

      bool viewable  = false;
      bool preferred = false;

      map<ustring, GMimeContentType *> viewable_text = {
        { "plain", g_mime_content_type_new ("text", "plain") },
        { "html" , g_mime_content_type_new ("text", "html") }
      };

      GMimeContentType * preferred_type = viewable_text["plain"];

      static ustring all_text ();
      static ustring all_html ();
  };
}

