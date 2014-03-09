# pragma once

# include <vector>

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

      ustring body ();

      vector<refptr<Chunk>> kids;

      bool viewable = false;

  };
}

