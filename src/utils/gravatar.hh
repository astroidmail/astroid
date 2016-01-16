# pragma once

# include "proto.hh"
# include <vector>

namespace Astroid {
  class Gravatar {
    public:
      static const int MIN_SIZE = 1;
      static const int MAX_SIZE = 512;
      static const int DEFAULT_SIZE = 80;

      enum Default {
        NOT_FOUND,
        MYSTERY_MAN,
        IDENTICON,
        MONSTER_ID,
        WAVATAR,
        RETRO,
      };

      static std::vector<ustring> DefaultStr;

      static ustring get_image_uri (ustring, enum Default, int = Gravatar::DEFAULT_SIZE);


  };
}

