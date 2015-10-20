# pragma once

# include <vector>
# include <map>
# include <atomic>

# include <gmime/gmime.h>

# include "astroid.hh"
# include "proto.hh"

using namespace std;

namespace Astroid {
  class Crypto {
    public:
      Crypto (ustring protocol);
      ~Crypto ();

      bool get_ready ();
      bool is_gpg ();

      GMimeObject * decrypt_and_verify (GMimeObject * mo);
      bool verify_signature (GMimeObject * mo);
      bool encrypt (GMimeObject * mo);
      bool sign (GMimeObject * mo);

      bool decrypted = false;
      bool verified  = false;
      bool verify_tried  = false;
      bool decrypt_tried = false;

    private:
      bool ready = false;
      bool isgpg = false;

      bool create_gpg_context ();
      GMimeCryptoContext * gpgctx;
      ustring protocol;
      ustring gpgpath = "/usr/bin/gpg";
  };
}

