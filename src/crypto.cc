# include <glib.h>
# include <gmime/gmime.h>

# include <string>

# include <boost/algorithm/string.hpp>

# include "astroid.hh"
# include "config.hh"
# include "crypto.hh"
# include "utils/address.hh"

namespace Astroid {
  Crypto::Crypto (ustring _protocol) {
    using std::endl;
    config       = astroid->config ("crypto");
    gpgpath      = ustring (config.get<std::string> ("gpg.path"));
    always_trust = config.get<bool> ("gpg.always_trust");

    LOG (debug) << "crypto: gpg: " << gpgpath;

    protocol = _protocol.lowercase ();

    if ((protocol == "application/pgp-encrypted" ||
         protocol == "application/pgp-signature")) {

      isgpg = true;
      create_gpg_context ();

    } else {
      LOG (error) << "crypto: unsupported protocol: " << protocol;
      ready = false;
      return;
    }


    ready = true;
  }

  Crypto::~Crypto () {
    LOG (debug) << "crypto: deconstruct.";

    /* if (slist)        g_object_unref (slist); */
    /* if (rlist)        g_object_unref (rlist); */
    if (decrypt_res)  g_object_unref (decrypt_res);
    if (gpgctx)       g_object_unref (gpgctx);
  }

  GMimeObject * Crypto::decrypt_and_verify (GMimeObject * part) {
    using std::endl;
    LOG (debug) << "crypto: decrypting and verifiying..";
    decrypt_tried = true;

    if (!GMIME_IS_MULTIPART_ENCRYPTED (part)) {
      LOG (error) << "crypto: part is not encrypted.";
      return NULL;
    }

    GError *err = NULL;

    GMimeMultipartEncrypted * ep = GMIME_MULTIPART_ENCRYPTED (part);
    GMimeObject * dp = g_mime_multipart_encrypted_decrypt
	(ep, gpgctx, &decrypt_res, &err);

    /* GMimeDecryptResult and GMimeCertificates
     *
     * Only the certificates of the signature are fully populated, the certificates
     * listed in GMimeDecryptResult->recipients are only a list of the alleged
     * key ids of the receivers. These can be spoofed by the sender, or normally set to
     * 0x0 if the receivers should be anonymous (-R for gpg).
     *
     * It is not possible not have a certificate entry for a receiver, so it is always
     * possible to see how many recipients there are for a message.
     *
     * For decrypted messages the certificate only has the key_id field set, the rest
     * of the information has to be fetched manually (note that the key_id is for the
     * subkey that supports encryption - E). When the rest of the information is fetched
     * from an available public key we cannot be sure that this is really the recipient.
     *
     *
     * Encryption and Decryption:
     *
     * The option always_trust on the gpg context must be set if the local key is not
     * trusted (or considered valid). Otherwise it is not used for encryption-target.
     *
     * There is some confusion about the term 'trust' here since it normally means whether
     * you trust the keys signing of other keys, in this case it means 'is the key valid'
     * 'or is the key really the one from the recipient'.
     *
     */

    if (decrypt_res) {
      rlist = g_mime_decrypt_result_get_recipients (decrypt_res);
      slist = g_mime_decrypt_result_get_signatures (decrypt_res);

      for (int i = 0; i < g_mime_certificate_list_length (rlist); i++) {

        GMimeCertificate * ce = g_mime_certificate_list_get_certificate (rlist, i);

        const char * c = NULL;
        ustring fp = (c = g_mime_certificate_get_fingerprint (ce), c ? c : "");
        ustring nm = (c = g_mime_certificate_get_name (ce), c ? c : "");
        ustring em = (c = g_mime_certificate_get_email (ce), c ? c : "");
        ustring key = (c = g_mime_certificate_get_key_id (ce), c ? c : "");

        LOG (debug) << "cr: encrypted for: " << nm << "(" << em << ") [" << fp << "] [" << key << "]";
      }
    }

    if (dp == NULL) {
      LOG (error) << "crypto: failed to decrypt message: " << err->message;
      decrypted = false;
      decrypt_error = err->message;

    } else {
      LOG (info) << "crypto: successfully decrypted message.";
      decrypted = true;

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

  bool Crypto::encrypt (GMimeObject * mo, bool sign, ustring userid, InternetAddress * from, ustring to, GMimeMultipartEncrypted ** out, GError ** err)
  {
    return encrypt (mo, sign, userid, from, AddressList (to), out, err);
  }

  bool Crypto::encrypt (GMimeObject * mo, bool sign, ustring userid, InternetAddress * from, AddressList to, GMimeMultipartEncrypted ** out, GError ** err)
  {

    /* build receipients */
    AddressList recp  = to + Address (from);
    recp.remove_duplicates ();

    GPtrArray * recpa = g_ptr_array_sized_new (recp.size ());

    std::vector<ustring> ur;

    for (Address &a : recp.addresses) {
      ur.push_back (a.email ());
    }

    LOG (debug) << "cr: encrypting for: ";
    for (ustring &u : ur) {
      g_ptr_array_add (recpa, (gpointer) u.c_str ());
      LOG (debug) << u << " ";
    }

    *out = g_mime_multipart_encrypted_new ();

    int r = g_mime_multipart_encrypted_encrypt (
        *out,
        mo,
        gpgctx,
        sign,
        userid.c_str (),
        GMIME_DIGEST_ALGO_DEFAULT,
        recpa,
        err);


    g_ptr_array_free (recpa, true);

    if (r == 0) {
      LOG (debug) << "crypto: successfully encrypted message.";
    } else {
      LOG (debug) << "crypto: failed to encrypt message: " << (*err)->message;
    }

    return (r == 0);
  }

  bool Crypto::sign (GMimeObject * mo, ustring userid, GMimeMultipartSigned ** out, GError ** err) {
    *out = g_mime_multipart_signed_new ();

    int r = g_mime_multipart_signed_sign (
        *out,
        mo,
        gpgctx,
        userid.c_str (),
        GMIME_DIGEST_ALGO_DEFAULT,
        err);

    if (r == 0) {
      LOG (debug) << "crypto: successfully signed message.";
    } else {
      LOG (debug) << "crypto: failed to sign message: " << (*err)->message;
    }

    return (r == 0);
  }

  bool Crypto::create_gpg_context () {

    if (!astroid->in_test ()) {

      gpgctx = g_mime_gpg_context_new (NULL, gpgpath.length() ? gpgpath.c_str () : "gpg");
      g_mime_gpg_context_set_use_agent ((GMimeGpgContext *) gpgctx, TRUE);
      g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, always_trust);

    } else {

      LOG (debug) << "crypto: in test";
      gpgctx = g_mime_gpg_context_new (NULL, "gpg");
      g_mime_gpg_context_set_use_agent ((GMimeGpgContext *) gpgctx, TRUE);
      g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, TRUE);

    }

    if (! gpgctx) {
      LOG (error) << "crypto: failed to create gpg context.";
      return false;
    }


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


