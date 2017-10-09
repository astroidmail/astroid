#! /usr/bin/env bash
#
# This sets up the test environment in the build root.
#

set -ep

src=$1
bld=$(realpath "$2")

echo "setting up test suite:"
echo "======================"

echo "* stopping all gpg components.."
gpgconf --kill all # stop all components

if [ -e $bld ]; then
  echo "test suite already set up."
  echo "======================"
  exit 0;
fi


echo "source root ....: $src"
echo "build root .....: $bld"


echo "* setting up notmuch test db.."

notmuch_db="${bld}/mail/test_mail"
mkdir -p "${notmuch_db}"

export NOTMUCH_CONFIG="${bld}/mail/test_config"

echo "* writing notmuch config: ${NOTMUCH_CONFIG}"
cp "${src}/mail/test_config.template" "${NOTMUCH_CONFIG}"
notmuch config set database.path "${notmuch_db}"

echo "* install e-mail test corpus.."
cp    ${src}/mail/*.eml ${notmuch_db}/..
cp -r "${src}/mail/test_mail/"* "${notmuch_db}/"
notmuch new

echo "* set up test scripts.."
cp "${src}/forktee"* "${bld}/"

echo "* set up test home.."
cp -r "${src}/test_home" "${bld}/"

echo "* set up theme.."
mkdir -p $bld/../ui
cp   $src/../ui/thread-view.* $bld/../ui
if [ -e $bld/../thread-view.css ]; then
  cp $bld/../thread-view.css $bld/../ui/
fi


echo "----------------------"
echo "* setting up GPG.."

export GNUPGHOME="${bld}/test_home/gnupg"
mkdir -p ${GNUPGHOME}
chmod og-rwx ${GNUPGHOME}

# gpg components have been stopped at top

cp ${src}/*.key ${bld}/

pushd ${GNUPGHOME}
gpg --batch --gen-key ../../foo1.key
gpg --batch --gen-key ../../foo2.key
gpg --batch --always-trust --import one.pub
gpg --batch --always-trust --import two.pub
echo "always-trust" > gpg.conf
popd

echo "======================"

