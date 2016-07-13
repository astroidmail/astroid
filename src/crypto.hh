# pragma once

# include <gmime/gmime.h>
# include <boost/property_tree/ptree.hpp>

# include "astroid.hh"
# include "proto.hh"

using boost::property_tree::ptree;

namespace Astroid {
  class Crypto : public Glib::Object {
    public:
      Crypto (ustring protocol);
      ~Crypto ();

      bool ready = false;
      bool isgpg = false;

      GMimeObject * decrypt_and_verify (GMimeObject * mo);
      bool verify_signature (GMimeObject * mo);
      bool encrypt (GMimeObject * mo, bool sign, ustring userid, InternetAddress * from, ustring to, GMimeMultipartEncrypted ** e);
      bool sign (GMimeObject * mo, ustring userid, GMimeMultipartSigned ** s);

      bool decrypted = false;
      bool verified  = false; /* signature ok */
      bool verify_tried  = false;
      bool decrypt_tried = false;

      GMimeDecryptResult * decrypt_res = NULL;
      GMimeSignatureList * slist = NULL;
      GMimeCertificateList * rlist = NULL;

    private:
      bool create_gpg_context ();
      GMimeCryptoContext * gpgctx = NULL;
      ustring protocol;
      ustring gpgpath;
      bool    auto_key_retrieve = false;
      ptree config;

      bool verify_signature_list (GMimeSignatureList *);

    public:
      static ustring get_md5_digest (ustring str);
      static unsigned char * get_md5_digest_char (ustring str);
  };

}

