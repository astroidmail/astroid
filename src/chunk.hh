# pragma once

# include <vector>
# include <string>

# include <gmime/gmime.h>

# include "astroid.hh"

using namespace std;

namespace Astroid {
  class Chunk : public Glib::Object {
    public:
      Chunk (GMimeObject *);
      ~Chunk ();

      /* Chunk takes ownership of these */
      GMimeObject * mime_object;
      GMimeContentType * content_type;

      ustring body ();

  };

  class TextChunk : public Chunk {

  };
}

