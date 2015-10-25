# include "gravatar.hh"
# include "openssl/md5.h"

# include "proto.hh"
# include "log.hh"

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
    unsigned char * tmp_hash;
    tmp_hash = MD5 ((const unsigned char *)addr.c_str (), addr.length (), NULL);

    ostringstream os;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
      os << hex << setfill('0') << setw(2) << ((int)tmp_hash[i]);
    }

    ustring hash = os.str ();

    ustring uri = ustring::compose (
          "http://www.gravatar.com/avatar/%1?d=%2&s=%3",
          hash,
          Gravatar::DefaultStr[def],
          size);

    log << debug << "gravatar: for: " << addr << ", uri: " << uri << endl;

    return uri;
  }
}

