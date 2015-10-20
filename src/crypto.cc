# include <glib.h>
# include <gmime/gmime.h>

# include <boost/algorithm/string.hpp>

# include "astroid.hh"
# include "log.hh"
# include "config.hh"
# include "crypto.hh"

/* interface to cryto, based on libnotmuch interface */

namespace Astroid {
  Crypto::Crypto (ustring _protocol) {
    ptree config = astroid->config->config.get_child ("crypto");
    gpgpath = ustring (config.get<string> ("gpg.path"));

    log << debug << "crypto: gpg: " << gpgpath << endl;

    protocol = _protocol.lowercase ();

    if ((protocol == "application/pgp-encrypted" ||
        protocol == "application/pgp-signature")) {

      isgpg = true;
      create_gpg_context ();

    } else {
      log << error << "crypto: unsupported protocol: " << protocol << endl;
      ready = false;
      return;
    }


    ready = true;
  }

  Crypto::~Crypto () {
    g_object_unref (gpgctx);
  }

  GMimeObject * Crypto::decrypt_and_verify (GMimeObject * part) {
    log << debug << "crypto: decrypting and verifiying.." << endl;
    decrypt_tried = true;
    verify_tried = true;

    if (!GMIME_IS_MULTIPART_ENCRYPTED (part)) {
      log << error << "crypto: part is not encrypted." << endl;
      return NULL;
    }

    GError *err = NULL;
    GMimeDecryptResult * res = NULL;

    GMimeMultipartEncrypted * ep = GMIME_MULTIPART_ENCRYPTED (part);
    GMimeObject * dp = g_mime_multipart_encrypted_decrypt
	(ep, gpgctx, &res, &err);

    if (dp == NULL) {
      log << error << "crypto: failed to decrypt message." << endl;
    } else {
      decrypted = true;
    }


    log << error << "crypto: successfully decrypted message." << endl;
    return dp;
  }


  bool Crypto::create_gpg_context () {
    gpgctx = g_mime_gpg_context_new (NULL, gpgpath.length() ? gpgpath.c_str () : "gpg");
    if (! gpgctx) {
      log << error << "crypto: failed to create gpg context." << endl;
      return false;
    }

    g_mime_gpg_context_set_use_agent ((GMimeGpgContext *) gpgctx, TRUE);
    g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, FALSE);

    return true;
  }
}


