#! /usr/bin/env bash
#
# wrapper for test nm standalone since we expect failure

function die() {
  echo "error: $1"
  exit 1
}

d=$(dirname $0)

echo "testing: test_notmuch_standalone: expecting failure.."
$d/test_notmuch_standalone && die "expected test_nm_standalone to fail!" 

exit 0

