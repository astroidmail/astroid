#! /usr/bin/env bash
#
# wrapper for test nm standalone since we expect failure

function die() {
  echo "error: $1"
  exit 1
}

function warn() {
  echo "warn: $1"
}

d=$(dirname $0)

# apparently, sometimes it does not fail.

echo "testing: test_notmuch_standalone: expecting failure.."
$d/test_notmuch_standalone && warn "expected test_nm_standalone to fail!"

exit 0

