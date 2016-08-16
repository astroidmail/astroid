# include "gravatar.hh"

# include "proto.hh"
# include "crypto.hh"

# include <iomanip>
# include <iostream>

using namespace std;

namespace Astroid {

  vector<ustring> Gravatar::DefaultStr = {
    "404",
    "mm",
    "identicon",
    "monsterid",
    "wavatar",
    "retro",
  };

/** Ported from Geary:
 *
 * Returns a URI for the mailbox address specified.  size may be any value from
 * MIN_SIZE to MAX_SIZE, representing pixels.  This function does not attempt
 * to clamp size to this range or return an error of any kind if it's outside
 * this range.
 *
 * TODO: More parameters are available and could be incorporated.  See
 * https://en.gravatar.com/site/implement/images/
 */
  ustring Gravatar::get_image_uri (
      ustring addr,
      enum Default def,
      int size)
  {
    ustring hash = Crypto::get_md5_digest (addr.c_str ());

    ustring uri = ustring::compose (
          "https://www.gravatar.com/avatar/%1?d=%2&s=%3",
          hash,
          Gravatar::DefaultStr[def],
          size);

    LOG (debug) << "gravatar: for: " << addr << ", uri: " << uri;

    return uri;
  }
}

