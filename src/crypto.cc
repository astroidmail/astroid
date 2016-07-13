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
    config = astroid->config ("crypto");
    gpgpath = ustring (config.get<std::string> ("gpg.path"));
    auto_key_retrieve = config.get<bool> ("gpg.auto_key_retrieve");
    always_trust = config.get<bool> ("gpg.always_trust");

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
    log << debug << "crypto: deconstruct." << endl;
  }

  GMimeObject * Crypto::decrypt_and_verify (GMimeObject * part) {
    using std::endl;
    log << debug << "crypto: decrypting and verifiying.." << endl;
    decrypt_tried = true;

    if (!GMIME_IS_MULTIPART_ENCRYPTED (part)) {
      log << error << "crypto: part is not encrypted." << endl;
      return NULL;
    }

    GError *err = NULL;

    GMimeMultipartEncrypted * ep = GMIME_MULTIPART_ENCRYPTED (part);
    GMimeObject * dp = g_mime_multipart_encrypted_decrypt
	(ep, gpgctx, &decrypt_res, &err);

    if (dp == NULL) {
      log << error << "crypto: failed to decrypt message: " << err->message << endl;
      decrypted = false;

    } else {
      log << info << "crypto: successfully decrypted message." << endl;
      decrypted = true;

      rlist = g_mime_decrypt_result_get_recipients (decrypt_res);
      slist = g_mime_decrypt_result_get_signatures (decrypt_res);

      for (int i = 0; i < g_mime_certificate_list_length (rlist); i++) {

        GMimeCertificate * ce = g_mime_certificate_list_get_certificate (rlist, i);

        const char * c = NULL;
        ustring fp = (c = g_mime_certificate_get_fingerprint (ce), c ? c : "");
        ustring nm = (c = g_mime_certificate_get_name (ce), c ? c : "");
        ustring em = (c = g_mime_certificate_get_email (ce), c ? c : "");
        ustring key = (c = g_mime_certificate_get_key_id (ce), c ? c : "");

        log << debug << "cr: encrypted for: " << nm << "(" << em << ") [" << fp << "] [" << key << "]" << endl;
      }

      verify_tried = (slist != NULL);
      verified = verify_signature_list (slist);
    }

    return dp;
  }

  bool Crypto::verify_signature (GMimeObject * mo) {
    GError * err = NULL;

    verify_tried = true;

    slist = g_mime_multipart_signed_verify (GMIME_MULTIPART_SIGNED(mo), gpgctx, &err);

    verified = verify_signature_list (slist);

    return verified;
  }

  bool Crypto::verify_signature_list (GMimeSignatureList * list) {
    if (list == NULL) return false;

    bool res = g_mime_signature_list_length (list) > 0;

    for (int i = 0; i < g_mime_signature_list_length (list); i++) {
      GMimeSignature * s = g_mime_signature_list_get_signature (list, i);

      res &= g_mime_signature_get_status (s) == GMIME_SIGNATURE_STATUS_GOOD;
    }

    return res;
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

    log << debug << "cr: encrypting for: ";
    for (ustring &u : ur) {
      g_ptr_array_add (recpa, (gpointer) u.c_str ());
      log << u << " ";
    }
    log << endl;

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
    g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, always_trust);
    g_mime_gpg_context_set_auto_key_retrieve ((GMimeGpgContext *) gpgctx, auto_key_retrieve);

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


