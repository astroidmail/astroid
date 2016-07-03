# include <glib.h>
# include <gmime/gmime.h>

# include <string>

# include <boost/algorithm/string.hpp>

# include "astroid.hh"
# include "log.hh"
# include "config.hh"
# include "crypto.hh"
# include "utils/address.hh"

/* interface to cryto, based on libnotmuch interface */
using std::endl;

namespace Astroid {
  Crypto::Crypto (ustring _protocol) {
    using std::endl;
    ptree config = astroid->config ("crypto");
    gpgpath = ustring (config.get<std::string> ("gpg.path"));

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
    using std::endl;
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

  bool Crypto::encrypt (GMimeObject * mo, bool sign, ustring userid, InternetAddress * from, ustring to, GMimeMultipartEncrypted ** out)
  {

    /* build receipients */
    AddressList recp (to);
    recp += Address (from);

    GPtrArray * recpa = g_ptr_array_sized_new (recp.size ());

    std::vector<ustring> ur;

    for (Address &a : recp.addresses) {
      ur.push_back (a.email ());
    }

    for (ustring &u : ur) {
      g_ptr_array_add (recpa, (gpointer) u.c_str ());
    }

    *out = g_mime_multipart_encrypted_new ();

    GError *err = NULL;

    int r = g_mime_multipart_encrypted_encrypt (
        *out,
        mo,
        gpgctx,
        sign,
        userid.c_str (),
        GMIME_DIGEST_ALGO_DEFAULT,
        recpa,
        &err);


    g_ptr_array_free (recpa, false);

    if (r == 0) {
      log << debug << "crypto: successfully encrypted message." << endl;
    } else {
      log << debug << "crypto: failed to encrypt message: " << err->message << endl;
    }

    return (r == 0);
  }

  bool Crypto::sign (GMimeObject * mo, ustring userid, GMimeMultipartSigned ** out) {
    *out = g_mime_multipart_signed_new ();

    GError *err = NULL;

    int r = g_mime_multipart_signed_sign (
        *out,
        mo,
        gpgctx,
        userid.c_str (),
        GMIME_DIGEST_ALGO_DEFAULT,
        &err);

    if (r == 0) {
      log << debug << "crypto: successfully signed message." << endl;
    } else {
      log << debug << "crypto: failed to sign message: " << err->message << endl;
    }

    return (r == 0);
  }

  bool Crypto::create_gpg_context () {
    gpgctx = g_mime_gpg_context_new (NULL, gpgpath.length() ? gpgpath.c_str () : "gpg");
    if (! gpgctx) {
      log << error << "crypto: failed to create gpg context." << std::endl;
      return false;
    }

    g_mime_gpg_context_set_use_agent ((GMimeGpgContext *) gpgctx, TRUE);
    g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, FALSE);

    return true;
  }

  ustring Crypto::get_md5_digest (ustring str) {
    unsigned char * digest = get_md5_digest_char (str);

    std::ostringstream os;
    for (int i = 0; i < 16; i++) {
      os << std::hex << std::setfill('0') << std::setw(2) << ((int)digest[i]);
    }

    delete digest;

    return os.str ();
  }

  unsigned char * Crypto::get_md5_digest_char (ustring str) {
    GMimeStream * mem = g_mime_stream_mem_new ();
    GMimeStream * filter_stream = g_mime_stream_filter_new (mem);

    GMimeFilter * md5f = g_mime_filter_md5_new ();
    g_mime_stream_filter_add(GMIME_STREAM_FILTER(filter_stream), md5f);

    g_mime_stream_write_string (filter_stream, str.c_str ());

    unsigned char *digest = new unsigned char[16];
    g_mime_filter_md5_get_digest (GMIME_FILTER_MD5(md5f), digest);


    g_object_unref (md5f);
    g_object_unref (filter_stream);
    g_object_unref (mem);

    return digest;
  }
}


