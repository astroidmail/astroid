#! /usr/bin/env bash
#
# Set up environment and run test specified on command line

set -ep

SRCDIR="${1}"
BINDIR="${2}"
TEST=${3}

echo "Source dir: ${SRCDIR}"
echo "Build dir:  ${BINDIR}"

export NOTMUCH_CONFIG="${BINDIR}/tests/mail/test_config"
export GNUPGHOME="${BINDIR}/tests/test_home/gnupg"
export ASTROID_BUILD_DIR="${BINDIR}"

gpgconf --kill all # stop all components

if [ ! -e "${NOTMUCH_CONFIG}" ]; then
  echo "Setting up test suite.."

  notmuch_db="${BINDIR}/tests/mail/test_mail"
  echo "setting up notmuch test db..: ${notmuch_db}"
  mkdir -p "${notmuch_db}"

  cp "${SRCDIR}/tests/mail/test_config.template" "${NOTMUCH_CONFIG}"
  notmuch config set database.path "${notmuch_db}"

  cp    "${SRCDIR}/tests/mail/"*.eml "${notmuch_db}/.."
  cp -r "${SRCDIR}/tests/mail/test_mail/"* "${notmuch_db}/"
  notmuch new

  cp    "${SRCDIR}/tests/forktee"* "${BINDIR}/tests/"
  cp -r "${SRCDIR}/tests/test_home" "${BINDIR}/tests/"

  # setup GPG
  mkdir -p "${GNUPGHOME}"
  chmod og-rwx "${GNUPGHOME}"

  pushd "${GNUPGHOME}"
  gpg --batch --gen-key "${SRCDIR}/tests/foo1.key"
  gpg --batch --gen-key "${SRCDIR}/tests/foo2.key"
  gpg --batch --always-trust --import one.pub
  gpg --batch --always-trust --import two.pub
  echo "always-trust" > gpg.conf
  popd

  # set up theme
  mkdir -p "${BINDIR}/ui"
  find "${SRCDIR}/ui/" \( -name "*.scss" -o -name "*.html" -o -name "*.css" \) -exec cp -v {} "${BINDIR}/ui/" \;
fi

echo "Running: ${TEST}.."

pushd "${BINDIR}"
./tests/${TEST}
popd


