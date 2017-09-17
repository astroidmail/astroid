# include "gmime-compat.h"

# include <stdexcept>

# if (GMIME_MAJOR_VERSION < 3)

void g_mime_message_add_mailbox (
    GMimeMessage * msg,
    GMimeAddressType t,
    const char * name,
    const char * addr)
{
  if (t == GMIME_ADDRESS_TYPE_SENDER || t == GMIME_ADDRESS_TYPE_FROM) {
    InternetAddress * a = internet_address_mailbox_new (name, addr);
    g_mime_message_set_sender (msg, internet_address_to_string (a, true, 0));
  } else {
    throw std::invalid_argument ("gmime-compat: not implemented addresstype");
  }
}

void g_mime_message_set_date (GMimeMessage * msg, GDateTime * dt) {
  # warning "gmime-compat: assuming timezone is local"
  GTimeZone * tz = g_time_zone_new_local ();
  g_mime_message_set_date (msg, g_date_time_to_unix (dt),  g_time_zone_get_offset (tz, 0));
  g_time_zone_unref (tz);
}

GDateTime * g_mime_message_get_date (GMimeMessage * msg) {
  time_t  time;
  g_mime_message_get_date (msg, &time, NULL);
  return g_date_time_new_from_unix_local (time);
}

GMimeMultipartEncrypted * g_mime_multipart_encrypted_encrypt (
    GMimeCryptoContext * gpgctx,
    GMimeObject * entity,
    gboolean sign,
    const char * userid,
    void * discard,
    GPtrArray * recp,
    GError ** err)
{
  (void) (discard);

  GMimeMultipartEncrypted * out = g_mime_multipart_encrypted_new ();

  int r = g_mime_multipart_encrypted_encrypt (
      out,
      entity,
      gpgctx,
      sign,
      userid,
      GMIME_DIGEST_ALGO_DEFAULT,
      recp,
      err);

  if (r != 0) {
    g_object_unref (out);
    return NULL;
  }

  return out;
}

GMimeMultipartSigned * g_mime_multipart_signed_sign (
    GMimeCryptoContext * gpgctx,
    GMimeObject * entity,
    const char * userid,
    GError ** err)
{
  GMimeMultipartSigned * out = g_mime_multipart_signed_new ();

  int r = g_mime_multipart_signed_sign (
      out,
      entity,
      gpgctx,
      userid,
      GMIME_DIGEST_ALGO_DEFAULT,
      err);

  if (r != 0) {
    g_object_unref (out);
    return NULL;
  }

  return out;
}

gboolean
g_mime_signature_status_good (GMimeSignatureStatus status) {
    return (status == GMIME_SIGNATURE_STATUS_GOOD);
}

gboolean
g_mime_signature_status_bad (GMimeSignatureStatus status) {
    return (status == GMIME_SIGNATURE_STATUS_BAD);
}

gboolean
g_mime_signature_status_error (GMimeSignatureError error) {
    return (error != GMIME_SIGNATURE_ERROR_NONE);
}

# else

gboolean
g_mime_signature_status_good (GMimeSignatureStatus status) {
    return ((status  & (GMIME_SIGNATURE_STATUS_RED | GMIME_SIGNATURE_STATUS_ERROR_MASK)) == 0);
}

gboolean
g_mime_signature_status_bad (GMimeSignatureStatus status) {
    return (status & GMIME_SIGNATURE_STATUS_RED);
}

gboolean
g_mime_signature_status_error (GMimeSignatureStatus status) {
    return (status & GMIME_SIGNATURE_STATUS_ERROR_MASK);
}

gint64
g_mime_utils_header_decode_date_unix (const char *date) {
  GDateTime* parsed_date = g_mime_utils_header_decode_date (date);
  time_t ret;

  if (parsed_date) {
    ret = g_date_time_to_unix (parsed_date);
    g_date_time_unref (parsed_date);
  } else {
    ret = 0;
  }

  return ret;
}

# endif



