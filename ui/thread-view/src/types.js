/**
 * A notmuch tag
 *
 * @typedef {Object} Tag
 *
 * @property {String} tag - name of the tag
 * @property {String} fg  - a css color expression for the foreground
 * @property {String} bg  - a css color expression for the background
 */

/**
 * A contact can be the sender or recipient of a message
 *
 * @typedef {Object} Contact
 *
 * @property {String} address        -
 * @property {String|undefined} name -
 */

/**
 * An object containing a date in useful formats
 *
 * @typedef {Object} AstroidDate
 *
 * @property {String} pretty    - short human readable
 * @property {String} verbose   - long human readable
 * @property {Number} timestamp - epoch timestamp
 */

/**
 * The mime content in a message, also known as the body
 *
 * @typedef {Object} MimeContent
 *
 * @property {String} mime_type            - example text/plain
 * @property {boolean} preferred           -
 * @property {boolean} encrypted           -
 * @property {boolean} signed              -
 * @property {String} content              - the content itself, may contain html
 * @property {Array<MimeContent>} children - child MimeContent
 */

/**
 *
 *
 * @typedef {Object} MimeMessage
 *
 * @property {String} message  - full body tree of message
 * @property {String} filename -
 * @property {String} size     - human readable size
 */

/**
 * Attachments to a message
 *
 * @typedef {Object} Attachment
 *
 * @property {String} filename             -
 * @property {String} size                 - human readable
 * @property {String} thumbnail            - base64 encoded image src with suitable size
 * @property {boolean} signed              - is attachment signed
 * @property {SignatureStatus} signature   - signature status (same as for body)
 * @property {boolean} encrypted           - is attachment encrypted
 * @property {EncryptionStatus} encryption - encryption status
 */

/**
 * Metadata regarding PGP encryption
 *
 * @typedef {Object} EncryptionStatus
 *
 * @property {String} name  -
 * @property {String} email -
 * @property {String} key   -
 */

/**
 * Metadata regarding the PGP signature
 *
 * @typedef {Object} SignatureStatus
 *
 * @property {boolean} verified            - whether the signature is good or not
 * @property {Array<Signature>} signatures - list of signatures
 */

/**
 * Metadata regarding the PGP signature
 *
 * @typedef {Object} Signature
 *
 * @property {String} name          -
 * @property {String} email         -
 * @property {String} key           -
 * @property {String} status        - one of good, bad, erroneous
 * @property {String} trust         - trust level of signers key (could also be reason for status != good)
 * @property {Array<String>} errors - indicating why status is not good
 */

/**
 * An email message as represented by Astroid
 *
 * @typedef {Object} Message
 *
 * @property {String} id                         -
 * @property {Array.<Contact>} from              -
 * @property {Array.<Contact>} to                -
 * @property {Array.<Contact>} cc                -
 * @property {Array.<Contact>} bcc               -
 * @property {String} in_reply_to                -
 * @property {AstroidDate} date                  -
 * @property {String} subject                    -
 * @property {String} gravatar                   - url to the avatar image
 * @property {Array.<Tag>} tags                  - list of tags on this message
 * @property {boolean} focused                   - whether the message is focused
 * @property {boolean} missing_content           - whether the message is missing content
 * @property {boolean} patch                     - whether the message is a patch and should have syntax highlighting applied to its body
 * @property {String} preview                    - plain text preview
 * @property {Array.<MimeContent>} body          - a list of MimeContent parts
 * @property {Array.<MimeMessage>} mime_messages -
 * @property {Array.<Attachment>} attachments    -
 */

