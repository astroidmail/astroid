/* gmime
 *
 * This provides a compatability layer between gmime 3 and gmime 2. Large parts likely to be
 * scavenged from notmuch gmime-extra.h
 *
 * */
# include <gmime/gmime.h>

# pragma once

# if (GMIME_MAJOR_VERSION < 3)

# define g_mime_init() g_mime_init(0)

# define g_mime_content_type_get_mime_type(c) g_mime_content_type_to_string(c)
# define g_mime_content_type_parse(f,c) g_mime_content_type_new_from_string(c)

# define g_mime_part_get_content(p) g_mime_part_get_content_object(p)
# define g_mime_part_set_content(p,w) g_mime_part_set_content_object(p,w)

# define g_mime_filter_dos2unix_new(f) g_mime_filter_crlf_new(f,false)
# define g_mime_object_write_to_stream(o,f,s) g_mime_object_write_to_stream (o,s)
# define g_mime_object_to_string(m,f) g_mime_object_to_string(m)

# define internet_address_to_string(ia,f,encode) internet_address_to_string(ia,encode)
# define internet_address_list_parse(f,str) internet_address_list_parse_string(str)
# define internet_address_list_to_string(ia,f,encode) internet_address_list_to_string(ia,encode)

# define g_mime_object_set_header(o,h,v,f) g_mime_object_set_header(o,h,v)
# define g_mime_utils_header_decode_text(NULL, n) g_mime_utils_header_decode_text(n)
# define g_mime_message_set_subject(m,s,f) g_mime_message_set_subject(m,s)

GDateTime * g_mime_message_get_date (GMimeMessage * msg);

# define GMIME_ADDRESS_TYPE_TO      GMIME_RECIPIENT_TYPE_TO
# define GMIME_ADDRESS_TYPE_CC      GMIME_RECIPIENT_TYPE_CC
# define GMIME_ADDRESS_TYPE_BCC     GMIME_RECIPIENT_TYPE_BCC
# define GMIME_ADDRESS_TYPE_FROM    (GMimeRecipientType)((GMIME_RECIPIENT_TYPE_BCC+1))
# define GMIME_ADDRESS_TYPE_SENDER  (GMimeRecipientType)((GMIME_ADDRESS_TYPE_FROM+1))
typedef GMimeRecipientType GMimeAddressType;
void g_mime_message_add_mailbox (GMimeMessage * msg, GMimeAddressType, const char *, const char *);
# define g_mime_message_get_addresses(m,t) g_mime_message_get_recipients(m,t)

# define g_mime_message_get_from(m) g_mime_message_get_sender(m)
# define g_mime_parser_construct_message(p,f) g_mime_parser_construct_message(p)

# define g_mime_stream_file_open(f,m,err) g_mime_stream_file_new_for_path(f,m)

/************************
 * Crypto
 ************************/
# define GMIME_DECRYPT_NONE NULL
# define g_mime_multipart_encrypted_decrypt(mpe, _de, _f, out, err) g_mime_multipart_encrypted_decrypt(mpe,gpgctx,out,err)
# define g_mime_crypto_context_decrypt(g, dopt, skey, is, os, err) g_mime_crypto_context_decrypt(g, is, os, err)

# define g_mime_multipart_signed_verify(mps, _e, err) g_mime_multipart_signed_verify(mps,gpgctx,err)

GMimeMultipartSigned * g_mime_multipart_signed_sign (
    GMimeCryptoContext * gpgctx,
    GMimeObject * entity,
    const char * userid,
    GError ** err);

# define GMIME_ENCRYPT_NONE NULL
GMimeMultipartEncrypted * g_mime_multipart_encrypted_encrypt (
    GMimeCryptoContext * gpgctx,
    GMimeObject * entity,
    gboolean sign,
    const char * userid,
    void * discard,
    GPtrArray * recp,
    GError ** err);

# else

typedef GMimeSignatureStatus GMimeSignatureError; // needed for status functions

# endif

gboolean g_mime_signature_status_good (GMimeSignatureStatus status);
gboolean g_mime_signature_status_bad (GMimeSignatureStatus status);
gboolean g_mime_signature_status_error (GMimeSignatureError status);

void g_mime_message_set_date_now (GMimeMessage * msg);

