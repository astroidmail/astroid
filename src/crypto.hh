# pragma once

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

namespace Astroid {
  class Crypto {
    public:
      Crypto (ustring protocol);
      ~Crypto ();

      bool ready = false;
      bool isgpg = false;

      GMimeObject * decrypt_and_verify (GMimeObject * mo);
      bool verify_signature (GMimeObject * mo);
      bool encrypt (GMimeObject * mo);
      bool sign (GMimeObject * mo);

      bool decrypted = false;
      bool verified  = false;
      bool verify_tried  = false;
      bool decrypt_tried = false;

    private:
      bool create_gpg_context ();
      GMimeCryptoContext * gpgctx;
      ustring protocol;
      ustring gpgpath;

    public:
      static ustring get_md5_digest (ustring str);
  };

}

