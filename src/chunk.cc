# include <vector>
# include <string>
# include <iostream>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "chunk.hh"

namespace Astroid {
  Chunk::Chunk (GMimeObject * mp) : mime_object (mp) {
    content_type = g_mime_object_get_content_type (mime_object);



  }

  ustring Chunk::body () {
    //return ustring(g_mime_object_to_string (mime_object));
    if (GMIME_IS_PART(mime_object)) {
      cout << "is part" << endl;

      GMimeDataWrapper * content = g_mime_part_get_content_object (
          (GMimePart *) mime_object);

      GMimeStream * stream = g_mime_stream_mem_new ();
      int s = g_mime_data_wrapper_write_to_stream (content, stream);

      char ss[s];
      g_mime_stream_read (stream, ss, s);

      cout << "body: " << ss << endl;
      return ustring(ss);
    } else {
      
      return ustring("not part");

    }
  }

  Chunk::~Chunk () {
    g_object_unref (mime_object);
    g_object_unref (content_type);
  }
}

