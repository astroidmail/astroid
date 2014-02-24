# include <vector>
# include <string>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "chunk.hh"

namespace Astroid {
  Chunk::Chunk (GMimeObject * mp) : mime_object (mp) {
    content_type = g_mime_object_get_content_type (mime_object);

  }

  ustring Chunk::body () {
    return ustring(g_mime_object_to_string (mime_object));
  }

  Chunk::~Chunk () {
    g_object_unref (mime_object);
    g_object_unref (content_type);
  }
}

